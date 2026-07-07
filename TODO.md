# MeltyFC — Build Tracker

## Legend
- [x] Done (implemented + tested)
- [ ] Not started
- [~] Partial / in progress

## P0: Skeleton
- [x] Project structure, platformio.ini, interfaces.h
- [x] Param registry + ConfigData struct (all §11 params)
- [x] Target skeleton (crux_f405hd pinmap PLACEHOLDER)
- [x] README, LICENSE, .clang-format, .gitignore
- [x] Bench test checklist (docs/BENCH_TEST.md)

## P1: Sensor Bringup (BLOCKED — needs Step 0 dump)
- [ ] Betaflight CLI dump → pinmap.h (Trip executes)
- [ ] I2C scan + dual H3LIS331 driver
- [ ] ICM-42688 SPI driver (gyro Z sign + accel)
- [ ] Minimal LED status (BOOT/SAFE/ERROR blink codes)

## P2: DShot Spike
- [x] DShot300 frame packing + 4-bit CRC
- [x] Timer compare-buffer encoding
- [x] Bidirectional GCR decode (5→4 bit lookup table)
- [x] Telemetry CRC validation + eRPM period extraction
- [x] DShot command enum
- [x] 25 unit tests (pack/CRC/timing/GCR/eRPM)
- [ ] HAL/LL timer+DMA send driver (BLOCKED — needs pin assignments)
- [ ] Bidir input-capture receive driver (BLOCKED — needs pin assignments)

## P3: Heading Engine + RPM Hold
- [x] Differential omega (dual accel)
- [x] Phase integration (wrapping)
- [x] N-motor output with translation windowing
- [x] Trim rate with expo curve
- [x] RPM hold (P + feedforward)
- [x] Inversion (phase sign negate)
- [x] Hit detection
- [x] Slew limiting
- [x] 30+ unit tests
- [ ] USB debug streaming (live omega/state over serial)

## Pure Logic — No Hardware Needed
- [ ] Config CLI parser (get/set/dump/save/defaults/list/list json/status/cal/version/help)
- [ ] Config store: schema migration logic (old blob → new, CRC validation)
- [ ] Config store: param registry get/set/format by ParamDef (populate stubs)
- [ ] CRSF frame parser (packet framing, channel extraction, CRC8 DVB-S2)
- [ ] CRSF telemetry frame builder (flight mode text, battery, custom sensor)
- [ ] LED state machine (priority stack, transitions, pattern timing logic)
- [ ] Creep/tank mode (differential drive math, N=2 full / N=3 degraded)
- [ ] Creep mode transition (hysteresis, CH_CREEP_FORCE switch)
- [ ] Heading re-sync gesture (stick angle averaging, threshold, offset snap)
- [ ] Hit logger (ring buffer, peak-G per hit, count)
- [ ] LVC (per-cell voltage calc, auto-detect cell count, warn/crit thresholds)
- [ ] Blackbox ring buffer (write pointer, sector management, CLI dump format)

## P4: CRSF + Safety (partial BLOCKED — UART needs pins)
- [x] Safety state machine (arming lifecycle)
- [x] 15 unit tests
- [ ] CRSF UART driver wiring (BLOCKED — needs pin assignments)
- [ ] Full LED state machine wiring (BLOCKED — needs LED DMA driver)

## P5: Translation + Inversion
- [x] Translation angle/magnitude from stick
- [x] Motor output with windowing
- [x] Inversion logic
- [ ] Stick-referenced heading re-sync integration
- [ ] Full integration on hardware (BLOCKED)

## P6: Config CLI + Param Registry
- [ ] CLI command parser + dispatch
- [ ] Registry enumeration (list / list json)
- [ ] get/set with type validation + clamping
- [ ] dump (diff-able/re-playable output)
- [ ] save/defaults (flash persistence)
- [ ] status (live telemetry output)
- [ ] cal (calibration mode entry)

## P7: Slip Detection + Blackbox
- [x] Slip math (eRPM → mechRPM → wheel omega → slip %)
- [x] 7 unit tests
- [ ] Blackbox ring buffer to SPI flash
- [ ] CLI dump command for blackbox

## P8: Creep, Hit Logger, LVC
- [ ] Creep mode integration
- [ ] Hit logger integration
- [ ] LVC warn/crit integration + auto spin-down

## P9: Telemetry, Tests, Docs
- [ ] CRSF telemetry: flight mode, battery, slip sensor
- [ ] Full test suite green
- [ ] Lint clean (clang-format + cppcheck)
- [ ] PARAMS.md generated from registry
- [ ] CLI_REFERENCE.md generated from registry
- [ ] TUNING.md

## P10: POV Display
- [ ] POV rendering engine (per-LED radial columns per revolution)
- [ ] Font/bitmap asset format definition
- [ ] Flash asset storage layout
- [ ] CLI upload protocol (chunked binary + checksum)
- [ ] Multi-layer compositing (beacon + POV + LVC overlay)
- [ ] Asset management (list/delete/upload via CLI)

## Hardware-Blocked Summary
Everything below this line requires the Step 0 Betaflight CLI dump from Trip:
- All sensor drivers (H3LIS I2C, ICM SPI)
- DShot HAL/LL timer+DMA (send + bidir receive)
- WS2812 HAL/LL DMA driver
- CRSF UART wiring
- VBAT ADC
- SPI flash (JEDEC probe)
- Full integration testing
