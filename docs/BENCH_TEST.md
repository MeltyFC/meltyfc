# MeltyFC Bench Test Checklist

## Prerequisites
- [ ] Smoke stopper on power supply
- [ ] Step 0 dump captured and committed (`docs/PINMAP_source_dump.txt`)
- [ ] Wheels OFF for all initial tests
- [ ] Firmware flashed via USB DFU

## P1: Sensor Bringup
- [ ] First power via smoke stopper — no shorts
- [ ] USB CDC serial connected, `version` command responds
- [ ] I2C scan shows 0x18 (inner H3LIS) and 0x19 (outer H3LIS)
- [ ] Both H3LIS reading reasonable values at rest (~0-1g)
- [ ] ICM-42688 gyro Z sign flips when board flipped by hand
- [ ] LED strip shows BOOT sweep, then SAFE breathe pattern
- [ ] Error blink codes display correctly when sensor disconnected

## P2: DShot
- [ ] DShot arming sequence — motors beep on power
- [ ] Motors spin at low DShot value (wheels OFF)
- [ ] All motors stop immediately on DShot 0
- [ ] Bidir DShot: eRPM telemetry received from at least one motor

## P4: CRSF + Safety
- [ ] CRSF channels visible in `status` output
- [ ] Arm gating: each precondition individually violated → no arm
  - [ ] Arm switch never seen LOW → blocked
  - [ ] Spin stick nonzero → blocked
  - [ ] RC link off → blocked
  - [ ] Sensor disconnected → blocked
- [ ] Arm → disarm → motors stop immediately
- [ ] Failsafe: TX off mid-spin (wheels off) → outputs 0 within FAILSAFE_MS + 50ms
- [ ] Failsafe recovery requires re-arm gesture (not just link return)
- [ ] Watchdog: deliberate hang → reset → boots DISARMED → motors 0
- [ ] LED states: SAFE, ARMED, FAILSAFE, ERROR all visually distinct

## P5: Translation (wheels ON, tethered)
- [ ] Low-speed tethered melty test
- [ ] LED beacon tracks consistent heading
- [ ] Translation responds to stick input
- [ ] Inversion: board flipped → beacon color changes, controls mirror correctly
- [ ] Heading re-sync gesture works

## P6: Config CLI
- [ ] `get`/`set` round-trip for all param types
- [ ] `dump` output is re-playable
- [ ] `save` persists across power cycle
- [ ] `defaults` resets everything
- [ ] `list json` produces valid JSON schema
- [ ] Bad input rejected with useful error messages

## Calibration
- [ ] dr_eff cal: reported RPM matches slow-mo video count
- [ ] Beacon position trim: LED points where translation goes
- [ ] Config mode entry/exit gesture works
- [ ] Calibration values persist after power cycle

## Acceptance
- [ ] All native unit tests green (`pio test -e native`)
- [ ] Lint clean (`pio check`)
- [ ] All §10 safety items demonstrated
- [ ] Calibration procedure documented as performed once on real hardware
