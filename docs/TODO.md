# MeltyFC TODO — Open Items
# Updated: 2026-07-09 (Round 5 fixes + G2 driver bodies + multi-target)

## Hardware-Blocked (waiting on boards)

- [ ] **Step 0: BF dump** — `dump all` + `resource list` on any board → diff against BF_CONFIG_DERIVED pinmaps
- [ ] **DMA stream/channel assignments** — verify from live dump, update HAL drivers
- [ ] **GPIO AF configuration** — verify alternate function mapping on real hardware
- [ ] **DMA TC interrupt wiring** — connect HAL callbacks to interrupt handlers
- [ ] **Edge-to-GCR timestamp conversion** — pure logic, extractable to common (blocked on having real capture data to validate against)
- [ ] **R3-4: I2C bus recovery** — clock SCL 9 times + STOP before Wire.begin. Target-specific GPIO bit-bang.
- [ ] **Clock configs** — real PLL trees (currently stub returning default HSI)
- [ ] **H743 MPU region activation** — code written in system_h7.cpp, needs hardware validation
- [ ] **Pinmap verification** — every BF_CONFIG_DERIVED pin must be diffed against live dump

## Integration Wiring (done when drivers exist on hardware)

- [ ] **D3: LVC → safety machine** — binding contract in main.cpp
- [ ] **D4: Creep arm gating** — binding contract in main.cpp
- [ ] **S1: Config storage → SPI flash** — A/B slots logic is pure. SPI driver needs hardware.

## Architectural (need Trip input)

- [ ] **Creep 3D mode** — full reverse requires Bluejay 3D config. v1 or fast-follow?

## Post-First-Spin (future firmware)

- [ ] **Dead motor detection + translation remap** — eRPM dropout → 2-motor remap (120° → 180°)
- [ ] **Lost wheel detection via slip monitoring** — slip >90% sustained >100ms → kill motor, remap
- [ ] **Lua telemetry dashboard** — CRSF flight mode string packing for TX16S display

## Completed

### Round 5 Multi-Target (2026-07-09)
- [x] T0: Pinmaps derived from BF unified configs (8464ca2)
- [x] T1: F722 HAL (Aikon F7 Mini) — build green (8464ca2)
- [x] T1b: F745 HAL (JHEMCU GHF745) — build green (8464ca2)
- [x] T2: H743 HAL (MicoAir H743 V2) — build green (d953f6d)
- [x] T3: Multi-target build system + boundary audit (8464ca2)
- [x] G1: verify.sh multi-target loop + per-target budgets (a9cfc1c)
- [x] G2: HAL driver bodies — DShot + WS2812 across F4/F7/H7 (d478640)
- [x] G3: PORTING.md (a9cfc1c)
- [x] G4: Boundary audit extended — cross-family include ban (a9cfc1c)
- [x] G5: Phase report / TODO updated (this commit)
- [x] G6: D2 constants unified, clock_config chip guard, MPU region 128KB (a9cfc1c)
- [x] S4: I2C decided for H3LIS331 (2026-07-08, Trip+Nexus)

### Earlier Rounds
- [x] C1: Motor thrust 90° offset (cd2a20d)
- [x] C2: Inversion phase direction (6f67e38)
- [x] A1-A8: All architectural findings (various commits)
- [x] B1-B5: All boundary findings (various commits)
- [x] D2, D5, D9: Delta findings (various commits)
- [x] S2, S3: Scope findings (various commits)
- [x] R3-2, R3-3, R3-5, R3-6: Round 3 findings (various commits)
- [x] Round 4 safety: 10 findings (6f67e38)
- [x] Data integrity: 22 findings (f622e32)
- [x] SIL sim: 7 scenarios (68b5c9d)

## Stats
- 245 tests, all passing (ASan/UBSan clean)
- 4 target builds: F405, F722, F745, H743
- 7 SIL sim scenarios
- 7-step verify chain (tests + 4-target build + budgets + cppcheck + clang-format + boundary + invariants)
- HAL drivers: DShot300 bidir + WS2812 across 3 families
