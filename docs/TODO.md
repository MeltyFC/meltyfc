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

- [ ] **S4: SPI vs I2C for H3LIS331** — SPI deletes bus budget problem (~5µs vs ~450µs). Requires 3 extra wires per sensor. Decision before soldering.
- [ ] **Creep 3D mode** — full reverse requires Bluejay 3D config. v1 or fast-follow?

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
