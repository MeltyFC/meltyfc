# MeltyFC TODO — Open Items
# Updated: 2026-07-07 after Round 4 safety fixes

## Critical (fix before board arrives)

- [ ] **C1: Motor position vs thrust 90° offset** — boost window centers on motor POSITION, but thrust is perpendicular (±90°). Needs sim scenario to verify and fix. Pin convention in code+docs.
- [x] **C2: Inversion phase direction** — ~~omega always positive, phase always increments~~ → integratePhase now takes direction param. SPIN_DIRECTION XOR inverted → signed phase. (6f67e38)
- [ ] **S1: Config storage → SPI flash** — internal flash writes stall CPU. Move config to SPI flash with A/B slots + CRC32 + sequence byte. Internal flash read-only (Invariant I-1). *Architectural — needs SPI flash driver.*
- [ ] **S3: USB CDC write guards** — Serial.print blocks with no host. Guard with availableForWrite check. Default streaming OFF.
- [ ] **A2: Degrees vs radians DerivedConfig** — registry stores degrees, engine consumes radians. Build conversion layer. Unit-test 30° → 0.5236 rad.
- [x] **A3: R_ICM parameter** — added to registry and param table. (6f67e38)
- [ ] **A6: Hit detection false-fires on spin-up** — gate to ω > threshold AND |commanded dω| below ramp limit.
- [ ] **Rocket invariant grep gates in verify.sh** — I-1 (FLASH_CR), I-2 (OPTCR), I-5 (CCM assert).

## Safety (Round 4 — completed 6f67e38)

- [x] **2a: LQ-based failsafe** — LQ==0 triggers FAILSAFE regardless of RC frames arriving. Defeats ELRS Last/Set Position modes.
- [x] **2b: Arm debounce** — 5 consecutive valid high frames required. Single low/invalid frame resets.
- [x] **2c: Valid-frame gating** — all preconditions only evaluated from CRC-valid, fresh frames. Default-init → all false.
- [x] **2g: Hot-plug disarm** — battery appears while armed → force DISARM.
- [x] **Choke point** — chokeMotorOutput() + throttleToDshot() enforce {0}∪[48,2047].
- [x] **T1-T8 safety tests** — 12 new unit tests covering the safety matrix.

## Important (fix before first spin)

- [ ] **S2: Creep forward-only** — clamp outputs to [0, cap], document no-reverse limitation.
- [ ] **S4: I2C bus budget** — two full reads = 450µs of 500µs loop. **Flag to Trip before soldering.**
- [x] **A1: validateConfig()** — spinMax>cap, rIn>rOut, LVC, windowHalf, channel collision, motors<2. (6f67e38)
- [x] **A4: Channel collision check** — folded into validateConfig(). (6f67e38)
- [ ] **A5: Bidir DShot EDT handshake** — cmd 13 ×6 while disarmed + line turnaround timing.
- [ ] **A7: Inversion × creep** — stickX negate when inverted.
- [x] **A8: NUM_DRIVE_MOTORS=1** — validated ≥2 in validateConfig(). (6f67e38)
- [x] **D2: First-iteration dt guard** — first loop uses nominal dt, subsequent clamped to 10ms max. (6f67e38)
- [ ] **D3: LVC → safety machine** — wire voltage into ArmPreconditions.
- [ ] **D4: Creep arm gating** — call site must gate on motorsAllowed().

## Cleanup (before v1 release)

- [ ] **B1: Schema version bump** — bump with layout changes + static_assert crc32 last.
- [ ] **B3: RESYNC_AVERAGE_MS** — implement recent-window or delete dead param.
- [ ] **B4: LVC cell-count ambiguity** — detect at boot only, document bands.
- [ ] **B5: CLI input-line bounding** — verify bounded buffer.
- [ ] **D5: RPM hold ramp on engage** — avoid throttle step.
- [ ] **D9: Trim rate discontinuity** — smooth deadband edge.
- [ ] **R3-2: HardFault handler** — motor kill + breadcrumbs to .noinit.
- [ ] **R3-3: CCM RAM assert** — runtime check at driver init.
- [ ] **R3-4: I2C bus recovery** — 9 SCL clocks + STOP at init.
- [ ] **R3-5: docs/RECOVERY.md** — DFU procedure for panicking humans.
- [ ] **R3-6: NVIC priorities** — explicit table.
- [ ] **SIL sim harness** — VERIFICATION.md §3. Required for P3 gate.

## Architectural (need Trip input)

- [ ] **S4: SPI vs I2C for H3LIS331** — decision before soldering.
- [ ] **Creep 3D mode** — v1 or fast-follow?
