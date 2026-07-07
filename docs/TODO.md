# MeltyFC TODO — Open Items
# Updated: 2026-07-07 (second pass — C1, A2, A7, D5, B1 done)

## Critical (fix before board arrives)

- [x] **C1: Motor thrust 90° offset** — boost window now uses thrust angle (position + π/2). Tests updated. (cd2a20d)
- [x] **C2: Inversion phase direction** — integratePhase takes direction param. (6f67e38)
- [ ] **S1: Config storage → SPI flash** — A/B slots + CRC logic is doable now, SPI driver needs hardware verification.
- [ ] **S3: USB CDC write guards** — guard writes with availableForWrite. Default streaming OFF.
- [x] **A2: DerivedConfig** — degrees→radians, mm→meters computed once at config load. (cd2a20d)
- [x] **A3: R_ICM parameter** — added to registry. (6f67e38)
- [x] **A6: Hit detection spin-up guard** — gated on ω > min AND |dω| < ramp. (3a4cfc5)
- [x] **Rocket invariant grep gates** — I-1, I-2 in verify.sh step 7. (3a4cfc5)

## Safety (Round 4 — all completed)

- [x] **2a: LQ-based failsafe** (6f67e38)
- [x] **2b: Arm debounce** — 5 consecutive frames (6f67e38)
- [x] **2c: Valid-frame gating** (6f67e38)
- [x] **2g: Hot-plug disarm** (6f67e38)
- [x] **Choke point + throttle mapping** (6f67e38)
- [x] **T1-T8 safety tests** (6f67e38)

## Important (fix before first spin)

- [x] **S2: Creep forward-only** — clamped to [0, cap]. (3a4cfc5)
- [ ] **S4: I2C bus budget** — **flag to Trip before soldering.**
- [x] **A1: validateConfig()** — full cross-param validation. (6f67e38)
- [x] **A4: Channel collision** — in validateConfig(). (6f67e38)
- [ ] **A5: Bidir DShot EDT handshake** — cmd 13 ×6, line turnaround.
- [x] **A7: Inversion × creep** — stickX negated when inverted. (cd2a20d)
- [x] **A8: NUM_DRIVE_MOTORS=1** — validated ≥2. (6f67e38)
- [x] **D2: First-iteration dt guard** (6f67e38)
- [ ] **D3: LVC → safety machine** — binding contract in main.cpp, wiring at integration.
- [ ] **D4: Creep arm gating** — binding contract in main.cpp, wiring at integration.

## Cleanup (before v1 release)

- [x] **B1: Schema version static_assert** — crc32 must be last field. (cd2a20d)
- [ ] **B3: RESYNC_AVERAGE_MS** — param exists, implementation deferred (needs time-based ring buffer).
- [ ] **B4: LVC cell-count ambiguity** — detect at boot, document bands.
- [ ] **B5: CLI input-line bounding** — verify bounded buffer.
- [x] **D5: RPM hold ramp on engage** — captures throttle on engagement. (cd2a20d)
- [x] **D9: Trim rate discontinuity** — remapped from deadband edge. (3a4cfc5)
- [ ] **R3-2: HardFault handler** — motor kill + breadcrumbs. Target-specific but fault struct is pure logic.
- [ ] **R3-3: CCM RAM assert** — one-liner in driver init.
- [ ] **R3-4: I2C bus recovery** — 9 SCL clocks at init.
- [x] **R3-5: docs/RECOVERY.md** — written. (3a4cfc5)
- [ ] **R3-6: NVIC priorities** — explicit table in target header.
- [ ] **SIL sim harness** — VERIFICATION.md §3. Required for P3 gate. Big task.

## Architectural (need Trip input)

- [ ] **S4: SPI vs I2C for H3LIS331** — decision before soldering.
- [ ] **Creep 3D mode** — v1 or fast-follow?
