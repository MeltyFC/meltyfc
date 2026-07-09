# MeltyFC — Bench Test & First-Flash Procedure
## Regenerated 2026-07-09 from Rounds 1-9 accumulated safety gates
## This is the document a human physically follows at first power.

---

## PREREQUISITES (before applying power)

- [ ] Smoke stopper in the power path (mandatory first flash)
- [ ] Props/wheels OFF
- [ ] ESC firmware updated (AM32 preferred, Bluejay acceptable)
- [ ] EP1 failsafe mode = "No Pulses" verified in ELRS WiFi UI
- [ ] Transmitter bound and channels verified

---

## STEP 0: PINMAP ARRIVAL-DIFF

For every BF_CONFIG_DERIVED target:
1. Connect FC via USB, open Betaflight CLI
2. Run `dump all` and `resource list`
3. Diff against `targets/<board>/pinmap.h` line by line
4. Update pinmap.h with any discrepancies
5. Remove `PINMAP_IS_BF_CONFIG_DERIVED` flag once verified
6. Rebuild and re-flash

**Do not proceed until the diff is clean.**

---

## STEP 1: FIRST POWER (smoke stopper)

1. Apply power through smoke stopper
2. Observe: smoke stopper lamp should NOT glow bright (= short)
3. Observe: status LED should be visible (boot succeeded)
4. If I-12 clock assert fires: fast blink, motors never arm -> check HSE crystal
5. USB CDC should enumerate -- connect serial terminal

---

## STEP 2: I2C SENSOR SCAN

1. CLI: `i2c scan` (when implemented)
2. Expected: 0x18 (H3LIS inner) + 0x19 (H3LIS outer) respond
3. If either missing: check wiring, solder joints, ADR jumper on outer sensor
4. R7-2: If I2C hangs -> bus recovery should fire (9x SCL + retry)
5. If bus recovery fails -> ERROR blink code, motors refuse to arm

---

## STEP 3: GYRO SIGN VERIFICATION

1. CLI: read gyro Z raw value
2. Hold FC right-side-up -> record sign
3. Flip FC upside-down -> sign should invert
4. If sign is wrong -> swap in firmware config
5. Debounce: hold each orientation for >150ms before reading

---

## STEP 4: ARM GATING (safety state machine)

### T2: Arm switch must have been LOW since boot
- Power on with arm switch HIGH -> must refuse to arm
- Cycle arm switch LOW then HIGH -> should arm (if all preconditions met)

### T3: Spin stick must be at minimum
- Move spin stick above 5% -> must refuse to arm
- Return to minimum -> should arm

### T11: Battery hot-plug
- Disconnect and reconnect battery while disarmed -> must stay DISARMED
- Must NOT auto-arm on battery reconnection

### Arming preconditions checklist:
- [ ] frameValid (receiving CRSF)
- [ ] linkQuality > 0
- [ ] batteryPresent (VBAT ADC reading valid)
- [ ] vbatValid (ADC initialized and in-range)
- [ ] lvcCritical = false
- [ ] configValid (loaded config passes validateConfig -- CO-1)
- [ ] Arm switch cycled LOW->HIGH
- [ ] Spin stick at minimum
- [ ] Arm debounce counter reaches 5 consecutive valid frames

---

## STEP 5: WHEELS-OFF SPIN TEST

1. Secure bot with wheels/props removed (or on a test stand)
2. Arm and apply minimal throttle
3. Observe: motors spin in correct direction
4. Observe: DShot telemetry reporting eRPM (if bidir enabled)
5. Disarm via arm switch -> motors stop immediately

---

## STEP 6: TX-OFF FAILSAFE (T9 -- CRITICAL SAFETY TEST)

1. Arm the bot
2. Apply ~50% throttle (motors spinning)
3. **Turn off the transmitter**
4. Observe: motors must stop within FAILSAFE_MS (default 500ms)
5. **TIME THIS WITH A STOPWATCH.** Record the measurement.
6. If motors continue beyond FAILSAFE_MS -> DO NOT FLY. Debug LQ/failsafe path.

---

## STEP 7: CPU-HALT BACKSTOP (T10 -- CRITICAL SAFETY TEST)

1. While armed and spinning at low throttle
2. Trigger a deliberate CPU halt (pull reset pin, or use debugger break)
3. Observe: motors must stop within ESC signal timeout
4. **MEASURE AND RECORD the ESC timeout value** (typically 250ms-1s)
5. This is I-3 + I-4: non-circular DMA = dead CPU = no more DShot frames = ESC stops
6. The ESC's own timeout is the last backstop -- know its value

---

## STEP 8: VBAT CALIBRATION (per-family -- R7-1)

1. Measure actual pack voltage with a multimeter
2. Compare against CLI-reported voltage
3. If off by more than 2%: adjust VBAT_DIVIDER_RATIO in pinmap.h
4. **H7 boards: verify 16-bit ADC is being used (not 12-bit)**
   - CLI reported voltage should be in the correct range
   - If voltage reads 16x too high -> ADC_FULL_SCALE is wrong
   - If voltage reads 16x too low -> divider ratio not applied

---

## STEP 9: dr_eff CALIBRATION

1. Mount bot on a turntable or free-spinning surface
2. Spin at low RPM with known heading reference (LED beacon)
3. Use slow-motion video to measure actual heading vs displayed heading
4. Adjust dr_eff (differential radius effective) until they match
5. Record the calibrated value

---

## STEP 10: EFFICIENCY (eta) CALIBRATION

1. Set bot spinning at a known throttle/voltage
2. Measure actual RPM (slow-motion video + heading engine readout)
3. Calculate: eta = measured_RPM / predicted_RPM (from calculator)
4. Enter measured eta in the calculator (locks the slider)
5. This calibrates all future KV/voltage/wheelbase calculations

---

## STEP 11: LED CURRENT CHECK

1. Set all LEDs to full white (maximum current draw)
2. Measure total current draw from the BEC/regulator
3. Verify within BEC budget (typically 1A for WS2812 strip)
4. If over budget: reduce LED count or max brightness in config

---

## STEP 12: FIRST TETHERED SPIN

1. Tether the bot (fishing line or similar)
2. Confirm heading beacon tracks correctly
3. Confirm translation responds to stick input
4. Confirm invertibility (flip bot, observe heading correction)
5. Confirm resync gesture works

---

## POST-TEST CHECKLIST

- [ ] EP1 failsafe = "No Pulses" verified
- [ ] TX-off failsafe timed: ___ms (must be <= FAILSAFE_MS)
- [ ] CPU-halt ESC timeout measured: ___ms
- [ ] VBAT calibrated against multimeter: ___V reported vs ___V actual
- [ ] dr_eff calibrated: ___ value
- [ ] eta calibrated: ___ value (entered in calculator)
- [ ] LED current within BEC budget: ___mA
- [ ] All arm gating tests passed
- [ ] Pinmap arrival-diff clean (BF_CONFIG_DERIVED removed)

---

## IF SOMETHING GOES WRONG

See [RECOVERY.md](RECOVERY.md) for DFU recovery, fault dump reading, and
emergency procedures. RECOVERY.md is current and multi-target aware.

---

*Regenerated from Rounds 1-9 accumulated safety gates. Previous version
predated the safety fault tree and was missing all gates added since Round 3.*
