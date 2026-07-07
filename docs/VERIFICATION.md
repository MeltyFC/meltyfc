# VERIFICATION.md — Agent Self-Verification Protocol
### MeltyFC firmware · applies to every phase, every commit · v1.0

## 0. PRINCIPLE

The implementing agent has a full Linux environment (Raspberry Pi) with shell access.
**Nothing is "done" on the agent's say-so — done means the verification chain below ran
and passed, on the Pi, with output shown.** The ONLY items exempt are the explicitly
hardware-gated checks in §5, which get handed to Trip with an exact procedure.

Claiming a phase complete without posting §6-format verification output is a process
violation, not a style issue.

## 1. ENVIRONMENT (one-time setup, self-serve)

```bash
pip install -U platformio
pio pkg install            # from repo root — pulls toolchains incl. arm-none-eabi
```
PlatformIO manages the ARM toolchain; no system compiler needed. If the Pi lacks
anything, install it and note it in the commit that needed it.

## 2. THE VERIFICATION CHAIN — run on EVERY meaningful change

One command, zero excuses:
```bash
./verify.sh
```
`verify.sh` (create in P0, keep green forever) runs, in order, failing fast:

1. **Native unit tests:** `pio test -e native`
   - native env builds with `-Wall -Wextra -Werror` AND **`-fsanitize=address,undefined`**
     (host-side ASan/UBSan is free bug-catching — memory errors and UB die here, not on
     the bot).
2. **Target build:** `pio run -e crux_f405hd`
   - Compiles the REAL firmware even with no hardware present — catches HAL misuse,
     missing symbols, linker issues immediately. A change that breaks the target build
     is broken, period.
3. **Resource budget:** parse the size output from step 2.
   - Gates: **flash < 70% of 1MB, RAM < 70% of 192KB** → warn; > 85% → FAIL and stop to
     discuss. (POV assets live on SPI flash, not in the image — if the image balloons,
     something is wrong.)
4. **Static analysis:** `pio check` (cppcheck) — zero new findings. Suppressions require
   an inline comment justifying each.
5. **Format:** `clang-format --dry-run --Werror` over `src/ include/ test/ targets/`.
6. **Architecture boundary audit** (grep-level, scriptable):
   ```bash
   ! grep -rlE '#include .*(stm32|hal_|cmsis|Arduino)' src/heading* src/slip* \
       include/param_registry.h
   ```
   Pure-logic modules containing hardware includes = FAIL. This enforces the §2 spec
   rule mechanically, not by review vibes.

## 3. SOFTWARE-IN-THE-LOOP SIM (the strongest no-hardware verification — build in P3)

Native-side **bot physics simulation** (`test/sim/`): a synthetic melty model that
closes the loop around the real heading engine and output stage:

- Plant model: omega dynamics from motor thrust (traction-capped), configurable
  geometry (uses the actual param registry).
- Sensor synth: dual-accel centripetal values at r_in/r_out + noise + **railing** +
  common-mode hit impulses; gyro Z sign w/ transient corruption on hits.
- Feed the REAL `heading.cpp` / output-window code (they're pure logic — that's why).

**Required sim scenarios (assert, don't eyeball):**
| scenario | assertion |
|---|---|
| spin-up to target | omega estimate tracks plant omega within tol; RPM hold converges, no overshoot oscillation |
| steady translation cmd | net displacement direction within ±10° of commanded φ over N revs (2-wheel AND 3-wheel) |
| hit impulse mid-translation | heading error bounded (differential cancels common-mode); gyro blanking window honored exactly |
| inversion event | mix mirrors; displacement still matches commanded φ; debounce rejects sub-threshold sign flicker |
| sensor railing | railed flag raised; spin governor caps; no NaN/garbage omega |
| stick re-sync gesture | heading_offset lands within stick-resolution tol of injected error |

Sim scenarios run inside `pio test -e native` — part of the chain, not a side quest.

## 4. PER-PHASE SELF-VERIFICATION (what the agent proves ALONE, before any hardware ask)

- **P0:** chain runs end-to-end green on the skeleton; verify.sh committed.
- **P1:** sensor drivers compile for target; register maps unit-tested against datasheet
  constants; I2C addresses parameterized (0x18/0x19).
- **P2:** DShot frame packer + GCR decoder pass known-vector tests (encode→decode
  round-trip incl. CRC corruption cases). Timing constants asserted against DShot300
  spec math in a test.
- **P3:** full sim suite (§3) green. USB debug stream format documented.
- **P4:** failsafe/arming state machine exhaustively unit-tested (every illegal
  transition rejected; CRSF timeout → outputs zero within FAILSAFE_MS in sim time).
- **P5:** translation + inversion sim scenarios green for N=2 and N=3.
- **P6:** CLI get/set/dump/save round-trip test; `list json` validates against a schema;
  config migration test (old blob → new schema).
- **P7:** slip math edges (no-slip, full-slip, ratio changes); blackbox ring-buffer
  wrap/dump tests.
- **P8–P10:** every new feature lands WITH its native/sim tests in the same commit.

## 5. HARDWARE GATES (the honest list of what the Pi CANNOT verify)

These — and ONLY these — go to Trip, each as a written procedure (steps, expected
result, what to capture):

1. Step 0 BF dump correctness / pin reality (P0)
2. I2C scan shows 0x18 + 0x19; sensor orientation SIGNS on the physical mount (P1)
3. DShot electrical timing on real ESC (motors beep/arm/spin; bidir eRPM returns) (P2)
4. Gyro sign convention upright vs inverted on the actual board orientation (P1/P4)
5. WS2812 on the dumped pin/timer renders (P1/P4)
6. CRSF from the real EP1 (channel order, failsafe behavior with TX off) (P4)
7. dr_eff calibration + reported-RPM vs slow-mo video (P3+ bench)
8. Anything RF, anything current/thermal, anything traction

Format each ask: **HARDWARE GATE [phase]: procedure / expected / capture-this.** Never
block on a hardware gate silently — post it and continue on non-dependent work.

## 6. PHASE REPORT FORMAT (posted at every phase gate — no report, no gate)

```
PHASE Pn REPORT
verify.sh: PASS (link/paste tail)
tests: X passed / 0 failed (native, incl. sim scenarios A,B,C)
target build: OK — flash XX.X% / RAM XX.X%
lint: clean (N suppressions, each justified inline)
boundary audit: clean
new tests added this phase: <list>
hardware gates now open for Trip: <list or none>
known gaps / deferred: <list or none>
```

## 7. REGRESSION RULES

- **Every bug fix ships WITH the test that would have caught it.** No naked fixes.
- The chain must be green at every commit on main. Broken chain = fix or revert before
  anything else.
- When a hardware gate result contradicts a sim assumption (e.g., real sensor noise
  differs), update the sim model in the same PR that consumes the finding — the sim
  only stays authoritative if it tracks reality.
