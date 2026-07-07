# MeltyFC TODO — Open Items

## Critical (fix before board arrives)

- [ ] **C1: Motor position vs thrust 90° offset** — boost window centers on motor POSITION, but thrust is perpendicular (±90°). Needs sim scenario to verify and fix. Pin convention in code+docs.
- [ ] **C2: Inversion phase direction** — omega is always positive (sqrt), phase always increments. When inverted, physical rotation reverses but software phase doesn't. Apply signed rotation direction (SPIN_DIRECTION XOR inverted) to phase integration.
- [ ] **S1: Config storage → SPI flash** — internal flash writes stall CPU, guaranteed watchdog reset during save. Move config to SPI flash with A/B slots + CRC32 + monotonic sequence byte + read-back-verify. Internal flash becomes read-only to application (Invariant I-1).
- [ ] **S3: USB CDC write guards** — `Serial.print` blocks when no host attached → loop stall → watchdog reset in arena. Guard every write with `if (Serial && Serial.availableForWrite() >= n)`. Default debug streaming OFF, explicit `stream on` CLI command.
- [ ] **A2: Degrees vs radians conversion layer** — registry stores degrees, engine consumes radians. No conversion exists. Build `DerivedConfig` struct computed once at config load/change. Unit-test conversions (30° → 0.5236 rad).
- [ ] **A3: R_ICM parameter missing** — low-speed mode needs ICM radius from spin axis. Add `R_ICM` (mm, runtime) to registry.
- [ ] **A6: Hit detection false-fires on spin-up** — during commanded ramp, true ω leads slew-limited estimate → residual exceeds threshold → gyro blanking triggers at every launch. Gate hit detection to ω > threshold AND |commanded dω| below ramp limit.
- [ ] **Rocket invariant grep gates in verify.sh** — I-1 (no FLASH_CR/FLASH_KEYR), I-2 (no OPTCR/OPTKEYR), I-5 (CCM assert). Add to verify.sh step 6.

## Important (fix before first spin)

- [ ] **S2: Creep descoped to forward-only** — DShot has no reverse without 3D mode. Clamp creep outputs to [0, cap]. Document limitation. 3D mode as fast-follow.
- [ ] **S4: I2C bus budget decision** — two full H3LIS reads = 450µs of 500µs loop. Options: read radial axis only (~260µs), wire sensors as SPI (~5µs), or I2C DMA. **Flag to Trip before soldering.**
- [ ] **A1: validateConfig() cross-param hook** — THROTTLE_SPIN_MAX > THROTTLE_CAP = reversed translation. Also: rInner < rOuter, LVC_WARN > LVC_CRIT, windowHalf ≤ 90°, channel collision check. Run at load + save + set.
- [ ] **A4: Channel-map collision check** — fold into validateConfig().
- [ ] **A5: Bidir DShot EDT handshake** — `DSHOT_CMD_BIDIR_EDT_MODE_ON` (cmd 13) sent ≥6× while disarmed. Add to arming sequence. Line turnaround timing (~25-30µs). GCR bit-order contract with real captured frame.
- [ ] **A7: Inversion × creep interaction** — stickX must negate when inverted in creep mode.
- [ ] **A8: NUM_DRIVE_MOTORS=1 validation** — forbid n=1 in validateConfig() or document/test degenerate behavior.
- [ ] **D2: First-iteration dt guard** — `lastLoopUs` starts 0 → huge first dt. Clamp: `if (dt > 0.01f) dt = LOOP_PERIOD_US * 1e-6f`.
- [ ] **D3: LVC not wired into safety machine** — ArmPreconditions needs voltage input. Decide: throttle override vs state transition at LVC_CRIT.
- [ ] **D4: Creep arm gating** — creepComputeOutput has no arm check. Call site MUST gate on motorsAllowed(state).

## Cleanup (before v1 release)

- [ ] **B1: Schema version bump** — any layout change to ConfigData must bump SCHEMA_VERSION. static_assert crc32 is last field.
- [ ] **B3: RESYNC_AVERAGE_MS dead parameter** — implement recent-window averaging or delete.
- [ ] **B4: LVC auto cell-count ambiguity** — 12.6-13.0V is both full 3S and dying 4S. Detect at boot only, document bands.
- [ ] **B5: CLI input-line bounding** — verify bounded line buffer with overflow-discard.
- [ ] **D5: RPM hold ramp on engage** — avoid throttle step when switching from manual to governor.
- [ ] **D9: Trim rate discontinuity** — computeTrimRate jumps 0→rateFine at deadband edge.
- [ ] **R3-2: HardFault handler** — force-disable motor timer+DMA, write fault breadcrumbs (PC/LR/CFSR) to .noinit RAM, await IWDG. Boot: if breadcrumbs present → ERROR blink + CLI fault dump.
- [ ] **R3-3: CCM RAM DMA assert** — runtime assert at driver init: `assert(((uint32_t)buf & 0xFFFF0000) != 0x10000000)`.
- [ ] **R3-4: I2C bus recovery at init** — clock SCL 9 times + STOP before Wire.begin.
- [ ] **R3-5: docs/RECOVERY.md** — DFU recovery procedure, written for someone panicking at 11pm.
- [ ] **R3-6: NVIC priorities** — explicit priority table for TIM/DMA (highest), I2C/SPI, USB/SysTick (lowest).
- [ ] **SIL sim harness** — VERIFICATION.md §3 bot physics simulation. Required for P3 gate.

## Architectural decisions (need Trip input)

- [ ] **S4: SPI vs I2C for H3LIS331** — SPI deletes the bus budget problem entirely (~5µs vs ~450µs). Requires 3 extra wires per sensor. Decision needed before Trip solders.
- [ ] **Creep 3D mode** — full reverse requires Bluejay 3D config + split-range throttle mapping + safety review. v1 or fast-follow?
