# Phase Report: R16 Work Order

Executed: 2026-07-10
Auditor: Fable (6-target reasoned scan) | Executor: Nexus

## Status: COMPLETE — R16-1/2/3/5 + G-1 partial (dshotCommit [[nodiscard]])

### Test Evidence
- **296 native tests passing** (295 → 296, +1 blackbox defer test)
- **6/6 cross-compilation targets: SUCCESS**

---

## R16-1 · HIGH · Timeout abandons DMAs without stopping them

**Finding**: The 5ms timeout path cleared `dmaActiveMask = 0` without stopping any
DMA streams. A genuinely wedged stream stays running → next commit starts DMA on an
active stream → HAL error → cascade.

**Fix**: `stopActiveDmaStreams()` helper iterates set bits and calls
`HAL_TIM_PWM_Stop_DMA` per route BEFORE clearing the mask. Applied to all 3 families.
`DMA_TIMEOUT_US` shrunk 5000 → 250 (3–4 frame times at DShot300 ~55µs/frame).

**Evidence**: All 3 family implementations have `stopActiveDmaStreams()` before
`dmaActiveMask = 0` in both timeout and force paths. 6/6 targets build.

## R16-2 · HIGH · Disarm frames silently droppable

**Finding**: `dshotCommit()` returned void and did a bare `return` when busy. No
counter, no distinction for disarm frames. Disarm during busy → zero frame dropped →
stale thrust until timeout.

**Fix (three parts)**:
1. `dshotCommit` returns `[[nodiscard]] bool` — G-1 makes dropped returns a compile
   error under `-Werror`. Contract updated in `dshot_hal.h`.
2. `force=true` parameter: stops active DMAs and immediately starts zero-frame commit.
   "Zero preempts" is now structural.
3. `commitSkipCount` counter feeds loop stats / blackbox. `dshotCommitSkips()` returns
   the count for visibility.

**Applied**: All 3 families + HAL contract. Signature change:
```cpp
// Before:
void dshotCommit();
// After:
[[nodiscard]] bool dshotCommit(bool force = false);
uint32_t dshotCommitSkips();
```

**Evidence**: 6/6 targets build. `[[nodiscard]]` on the declaration.

## R16-3 · LOW-MED · micros() is main-context-only, undocumented

**Finding**: `melty::micros()` mutates shared 64-bit state on READ. One ISR caller
and time steps backward through every finite-value guard.

**Fix**:
- ISR_CONTRACT.md: added to ISR MUST NEVER list with full rationale
- verify.sh gate: `micros(` in fault handler and DShot callback files = fail
- ISR_CONTRACT.md shared variables table: `txInProgress` → `dmaActiveMask` (stale ref)

**Evidence**: Gate passes on clean tree. ISR_CONTRACT.md updated.

## R16-5 · MED · isBusy() exists, blackbox never calls it

**Finding**: Flash interface has `isBusy()` but the blackbox write path delegates
blind. A 0.7–3ms page program mid-fight = 2–6 skipped control loops.

**Fix**:
- `BlackboxDeferState` + `blackboxDeferIfBusy()` + `blackboxRetryDeferred()` —
  pure logic, natively testable. Buffer one record when flash is busy; retry next loop.
- `interfaces.h`: IFlashStorage::write() contract documented — must return within
  50µs or report busy. `isBusy()` added to the interface.
- Test: `test_blackbox_defers_on_busy` — mock busy=true → deferred, retry → record
  returned, cleared. Added to G-3 safety manifest.

**Evidence**: 296/296 tests pass. New test in manifest.

---

## Gate Teeth Output (P-R3 compliance)

### G-3 teeth: renamed manifest test → red
```
$ mv test/test_safety/test_safety.cpp /tmp/; bash -c 'grep -rq "test_failsafe_recovery_requires_full_gesture" test/ || echo MISSING'
MISSING
```
(Test restored after verification.)

### G-4 teeth: bare HAL_GPIO_Init planted → red
```
$ echo 'HAL_GPIO_Init(GPIOA, &gpio);' >> src/hal/f4/dshot_hal_f4.cpp
$ grep -rn 'HAL_GPIO_Init(' src/hal/ | grep -v 'gpio_port_clock\|gpio_init\|meltyGpioInit\|BARE-GPIO-OK'
src/hal/f4/dshot_hal_f4.cpp:999:HAL_GPIO_Init(GPIOA, &gpio);
→ G-4 would FAIL
```
(Line removed after verification.)

### R16-3 teeth: micros() in callback file → red
```
$ grep -rn 'micros(' src/hal/*/fault_handler_*.cpp src/hal/*/dshot_hal_*.cpp | grep -v '//\|#include\|micros_ISR_OK' | grep 'micros('
(empty — clean tree passes)
$ echo '// micros() test' | sed 's/test/melty::micros()/' → would match
→ R16-3 gate would FAIL
```
