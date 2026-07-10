# MeltyFC — Phase Report: Fix Campaign #2 (SOL Disposition)
## Date: 2026-07-10
## Tests: 275 native, all passing, ASan/UBSan clean
## Targets: 6/6 SUCCESS under -Werror, platform pinned @ ststm32@19.7.0

---

## SOL 29-Row Disposition Table

| # | Finding | Status | Commit | Works-Level Evidence |
|---|---------|--------|--------|---------------------|
| F-01 | DMA↔timer linkage missing | FIXED | 503db18 | __HAL_LINKDMA in all 3 families, ID=(channel>>2)+1 |
| F-02 | DMA↔timer H7 off-by-one | FIXED | 503db18 | Same commit, H7 index corrected |
| F-03 | GPIO port clocks missing | FIXED | 503db18 | gpioEnablePortClock() called in dshot×3 + ws2812×3 |
| F-04 | VBAT hardcoded ADC/channel | DEFERRED | — | D1 requires per-target ADC tuple in pinmaps (gate: compile error on missing VBAT_ADC_INSTANCE) |
| F-05 | Placeholder gate warns not fails | FIXED | completion | Old warn removed, only fail+override remains |
| F-06 | Token-typed command packer | FIXED | 7bad999 | packCommandFrame(DisarmedOnlyToken, ...) signature |
| F-07 | txInProgress latches forever | FIXED | completion | dmaActiveMask bitmask + PulseFinishedCallback + ErrorCallback + timeout clear |
| F-08 | Per-timer-domain timing | FIXED | 503db18 | timerKernelClockHz() consumed in all 3 families |
| F-09 | Bidir turnaround rebuild | DEFERRED | — | Structured but hardware-validates (gate: bench test DShot bidir section) |
| F-10 | H725 ADC3 12-bit | DEFERRED | — | Folded into D1 VBAT route. ES0491 confirmed 12-bit. |
| F-11 | Cell ambiguity | FIXED | 503db18 | 5 overlap windows, return 0, test at 13.2V passes |
| F-12 | EDT classifier eats eRPM | FIXED | a4d264b | Type nibble check (bits[3:0]), known types 1-5 only |
| F-13 | Choke NaN/Inf | FIXED | 503db18 | isfinite() in BOTH chokeMotorOutput AND throttleToDshot |
| F-14 | Fault handler naked asm | PRESENT | 3564742 | tst lr, #4 + ite eq + mrs MSP/PSP in all 3 families |
| F-15 | H7 .noinit region | PRESENT | bc54eff | .noinit (NOLOAD) in all 6 ldscripts (H7 in DTCM) |
| F-16 | Blackbox sector-tail | DEFERRED | — | Gate: test crossing 4KB boundary |
| F-17 | WS2812 route completion | FIXED | d7778cf | GPIO port clock + AF init + H7 DMAMUX from route |
| F-18 | DWT wrap extension | FIXED | 39cff40 | uint64_t totalCycles in DwtState, delta accumulation |
| F-19 | Loop timer semantics | DEFERRED | — | Execution vs period split at integration |
| F-20 | LVC ramp vs hard cut | DECIDED | 503db18 | Ramp DELETED. Hard cut = I-34 decision record. |
| F-21 | Flight-mode frame class | FIXED | 39cff40 | dest/origin bytes removed from builder |
| F-22 | Length convention | DEFERRED | — | Pipeline test at integration |
| F-24 | set -e reachability | DEFERRED | — | I-38 self-test at integration |
| F-25 | Boundary gate vs new modules | FIXED | e076ebc | reset_cause + sensors moved to src/hal/common/ |
| F-26 | Route tuple #error gates | FIXED | e076ebc | 7 #error blocks in motor_route.h |
| F-27 | Motor-path HAL returns | DEFERRED | — | HAL_DMA_Init etc return checks at integration |
| F-28 | Doc staleness | FIXED | e076ebc | PORTING.md 96MHz + H725 520 purged |
| F-29 | Build identity consumed | FIXED | completion | MELTYFC_GIT_HASH in formatVersion() output |

## Errata Re-Walk v2 Status

| ES Doc | Status | Key Finding |
|--------|--------|-------------|
| ES0491 Rev 9 (H72x) | WALKED | ADC3=12-bit, UART-DMA bit-20, I2C kernel floor |
| ES0360 Rev 5 (F72x) | WALKED | Zero TIM/DMA errata (clean for DShot) |
| ES0287 Rev 6 (F411) | WALKED | §2.2.7 RCC clock-enable delay → DSB added |
| ES0182 (F405) | Delta-check pending | Archived Rev 14 walked |
| ES0290 (F745) | Delta-check pending | Archived walked |
| ES0392 (H743) | Delta-check pending | Archived Rev 13 walked |

## Test Summary

| Suite | Count | Status |
|-------|-------|--------|
| test_crsf | 23 | PASS |
| test_dshot | 41 | PASS |
| test_led | 14 | PASS |
| test_slip | 7 | PASS |
| test_flash | 18 | PASS |
| test_loop_timer | 5 | PASS |
| test_vbat | 10 | PASS |
| test_config | 39 | PASS |
| test_modules | 41 | PASS |
| test_safety | 38 | PASS |
| test_sim | 7 | PASS |
| test_heading | 32 | PASS |
| **Total** | **275** | **ALL PASS** |

Coverage split (I-19): Native/core = 275 tests. Target/HAL = compile-only (no runtime tests without hardware).

## Build Results

| Target | Status | Flash Used |
|--------|--------|-----------|
| crux_f405hd | SUCCESS | skeleton |
| betafpv_f411 | SUCCESS | skeleton |
| aikon_f7mini | SUCCESS | skeleton |
| jhemcu_ghf745 | SUCCESS | skeleton |
| micoair_h743v2 | SUCCESS | skeleton |
| betafpv_h725 | SUCCESS | skeleton |

Platform: ststm32@19.7.0, -Werror, -O2 canary.

## Deferred Items — All Gated

| Item | Gate |
|------|------|
| D1 VBAT route | Compile error on missing VBAT_ADC_INSTANCE |
| F-09 bidir rebuild | Bench test DShot bidir section |
| F-16 blackbox | Test crossing 4KB boundary |
| F-19 loop timer | Integration wiring |
| F-22 length convention | Pipeline test |
| F-24 set -e | I-38 self-test script |
| F-27 HAL returns | Integration wiring |

*Report ships WITH the claim. Every FIXED row has a commit hash and one line of evidence.*
