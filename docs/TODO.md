# MeltyFC TODO — Open Items
# Updated: 2026-07-07 (third pass — SIL sim, cleanup batch)

## Remaining (not hardware-blocked)

- [ ] **S1: Config storage → SPI flash** — A/B slots + CRC + sequence byte + read-back-verify. The storage LOGIC is pure (can write+test now). SPI flash DRIVER needs hardware.
- [ ] **A5: Bidir DShot EDT handshake** — DSHOT_CMD_BIDIR_EDT_MODE_ON (cmd 13) ×6 while disarmed. Add to arming sequence. Line turnaround timing (~25-30µs).
- [ ] **R3-2: HardFault handler** — force-disable motor timer+DMA at register level, write fault breadcrumbs (PC/LR/CFSR) to .noinit RAM, spin for IWDG. Boot: check breadcrumbs → ERROR blink + CLI fault dump. The fault struct and CLI dump formatter are pure logic.
- [ ] **R3-4: I2C bus recovery** — clock SCL 9 times + STOP before Wire.begin. Target-specific (GPIO bit-bang at init).

## Integration wiring (done when drivers exist)

- [ ] **D3: LVC → safety machine** — binding contract written in main.cpp. Wire at integration.
- [ ] **D4: Creep arm gating** — binding contract written in main.cpp. Wire at integration.

## Architectural (need Trip input)

- [x] **S4: SPI vs I2C for H3LIS331** — DECIDED: I2C. CruxF405HD has no external SPI pads available (SPI1=gyro, SPI2=flash, no breakout). I2C on PB10/PB11 shared bus with baro. ~450µs per 2-sensor read = ~2kHz loop. Workable. (2026-07-08, Trip+Nexus)
- [ ] **Creep 3D mode** — full reverse requires Bluejay 3D config. v1 or fast-follow?

## Post-First-Spin (future firmware)

- [ ] **Dead motor detection + translation remap** — Detect motor loss via bidir DShot eRPM dropout (eRPM=0 for >N cycles). Remap translation windows to surviving motors (3→2 motor map: 120° spacing → 180°). Accept reduced translation authority (~33% duty vs 50%). Mass imbalance from dead pod creates slight wobble — heading engine may need center-of-rotation offset. No hardware changes needed. (2026-07-08, Trip request)
- [ ] **Lost wheel detection via slip monitoring** — Detects PU wheel loss/delamination (motor healthy but no traction). Compare per-motor eRPM against expected eRPM from bot RPM (H3LIS) × gear ratio. If slip >90% sustained for >100ms → flag motor as lost-wheel, kill output, remap to 2-motor translation. Recovery logic: after ~5s, re-engage at low throttle, re-check slip. If grips → restore 3-motor. If still slipping → kill permanently for remainder of match. Same remap infrastructure as dead motor detection. (2026-07-08, Trip request)

## Completed (this session)

- [x] C1: Motor thrust 90° offset (cd2a20d)
- [x] C2: Inversion phase direction (6f67e38)
- [x] A1: validateConfig() cross-param (6f67e38)
- [x] A2: DerivedConfig degrees→radians (cd2a20d)
- [x] A3: R_ICM parameter (6f67e38)
- [x] A4: Channel collision check (6f67e38)
- [x] A6: Hit detection spin-up guard (3a4cfc5)
- [x] A7: Inversion × creep (cd2a20d)
- [x] A8: NUM_DRIVE_MOTORS validation (6f67e38)
- [x] B1: Schema version static_assert (cd2a20d)
- [x] B3: RESYNC_AVERAGE_MS documented (1f6fd88)
- [x] B4: LVC cell-count ambiguity documented (1f6fd88)
- [x] B5: CLI line buffer bounded (1f6fd88)
- [x] D2: First-iteration dt guard (6f67e38)
- [x] D5: RPM hold ramp on engage (cd2a20d)
- [x] D9: Trim rate discontinuity (3a4cfc5)
- [x] S2: Creep forward-only (3a4cfc5)
- [x] S3: USB CDC write guards (1f6fd88)
- [x] R3-3: CCM RAM assert macro (1f6fd88)
- [x] R3-5: RECOVERY.md (3a4cfc5)
- [x] R3-6: NVIC priority table (1f6fd88)
- [x] Rocket invariant grep gates I-1, I-2 (3a4cfc5)
- [x] SIL sim harness — 7 scenarios (68b5c9d)
- [x] Round 4 safety: LQ failsafe, arm debounce, valid-frame gating, hot-plug disarm, choke point (6f67e38)

## Stats
- 212 tests, all passing
- 7 SIL sim scenarios
- 7-step verify chain (tests + target build + budget + cppcheck + clang-format + boundary + invariants)
