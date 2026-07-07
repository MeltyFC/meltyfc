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
- [x] 25 unit tests
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
- [x] Config CLI parser (get/set/dump/save/defaults/list/list json/status/cal/version/help) — 36 tests
- [x] Config store: schema migration logic (old blob → new, CRC validation)
- [x] Config store: param registry get/set/format by ParamDef — 55 params fully populated
- [x] CRSF frame parser (packet framing, channel extraction, CRC8 DVB-S2) — 19 tests
- [x] CRSF telemetry frame builder (flight mode text, battery, custom sensor)
- [x] LED state machine (priority stack, transitions, pattern timing logic) — 14 tests
- [x] Creep/tank mode (differential drive math, N=2 full / N=3 degraded) — 8 tests
- [x] Creep mode transition (hysteresis, CH_CREEP_FORCE switch)
- [x] Heading re-sync gesture (stick angle averaging, threshold, offset snap) — 6 tests
- [x] Hit logger (ring buffer, peak-G per hit, count) — 5 tests
- [x] LVC (per-cell voltage calc, auto-detect cell count, warn/crit thresholds) — 8 tests
- [x] Blackbox ring buffer (write pointer, sector management, CLI dump format) — 8 tests

## P4: CRSF + Safety (partial BLOCKED — UART needs pins)
- [x] Safety state machine (arming lifecycle)
- [x] 15 unit tests
- [ ] CRSF UART driver wiring (BLOCKED — needs pin assignments)
- [ ] Full LED state machine wiring (BLOCKED — needs LED DMA driver)

## P5: Translation + Inversion
- [x] Translation angle/magnitude from stick
- [x] Motor output with windowing
- [x] Inversion logic
- [x] Stick-referenced heading re-sync — pure logic done, integration on hardware BLOCKED
- [ ] Full integration on hardware (BLOCKED)

## P6: Config CLI + Param Registry
- [x] CLI command parser + dispatch
- [x] Registry enumeration (list / list json)
- [x] get/set with type validation + clamping
- [x] dump (diff-able/re-playable output)
- [x] save/defaults (logic done — flash persistence BLOCKED on hardware)
- [x] status (format done — live data BLOCKED on hardware)
- [x] cal (entry logic — cal mode BLOCKED on hardware)

## P7: Slip Detection + Blackbox
- [x] Slip math (eRPM → mechRPM → wheel omega → slip %)
- [x] 7 unit tests
- [x] Blackbox ring buffer logic (write pointer, sector management, CLI dump format)
- [ ] Blackbox SPI flash driver (BLOCKED — needs hardware)
- [ ] CLI dump command for blackbox (logic done — I/O BLOCKED)

## P8: Creep, Hit Logger, LVC
- [x] Creep mode pure logic (differential drive, hysteresis transition)
- [x] Hit logger pure logic (ring buffer, peak-G tracking, format)
- [x] LVC pure logic (auto-detect cells, warn/crit, spin-down ramp)
- [ ] Integration with main loop (BLOCKED — needs hardware)

## P9: Telemetry, Tests, Docs
- [x] CRSF telemetry: flight mode frame builder
- [x] CRSF telemetry: battery sensor frame builder
- [x] Full test suite: 180 tests, 0 failures
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
