# MeltyFC Tuning Guide

## First-Time Setup

### 1. dr_eff Calibration (CRITICAL — do this first)

The `DR_EFF` parameter is the effective radial separation between your two H3LIS331
accelerometers. This is the single most important calibration constant — if it's wrong,
your RPM reading is wrong, and everything downstream (translation, governor) breaks.

**Procedure:**
1. Mount both H3LIS331 sensors on the same radial line from the spin axis
2. Measure approximate inner and outer radii → set `R_INNER` and `R_OUTER`
3. `DR_EFF` is auto-derived as `R_OUTER - R_INNER`, but needs empirical trim
4. Enter cal mode: hold full-back on spin stick while disarmed
5. Spin at a known fixed throttle (~50%)
6. Count LED beacon flashes per second on slow-mo video
7. Compare to firmware-reported RPM on `status` output
8. Adjust `DR_EFF` up/down with stick until they match
9. Exit cal mode — value persists

**Why the measurement isn't enough:** The effective radii depend on the exact mounting
position relative to the true spin axis, which is rarely perfectly centered. The
differential cancels most error, but `DR_EFF` still needs a 10-20% empirical trim.

### 2. Beacon Position Trim

After dr_eff cal, the beacon arc may not point at the true front of the bot.

1. Enter cal mode
2. Spin at low speed
3. Watch the beacon direction vs the bot's true front
4. Adjust heading offset with the trim stick
5. Exit cal mode — persists

### 3. Motor Pole Count

Set `MOTOR_POLES` to match your motor spec (count magnets, divide by 2, or check the
motor datasheet). This only matters for slip detection (eRPM → mechanical RPM).

Default: 14 (typical 1103/1105 size motor)

## Control Tuning

### Translation Window (`WINDOW_HALF`)

- **Wider (45-60°):** more translation authority, less spin stability
- **Narrower (15-25°):** more stable spin, less translation
- **Default 30°:** balanced starting point

Start at 30°, increase if the bot won't translate fast enough, decrease if it wobbles.

### Throttle Mapping (`THROTTLE_SPIN_MAX`, `THROTTLE_CAP`)

- `THROTTLE_SPIN_MAX` (default 0.75): maximum spin throttle from stick
- `THROTTLE_CAP` (default 0.90): absolute output cap including translation boost
- The gap (0.90 - 0.75 = 0.15) is reserved for translation modulation

If spin+translate clips, reduce `THROTTLE_SPIN_MAX` or increase `THROTTLE_CAP`.

### Heading Trim (`TRIM_RATE_FINE`, `TRIM_RATE_MAX`, `TRIM_EXPO`)

- `TRIM_RATE_FINE`: how fast trim moves at minimal stick deflection (deg/s)
- `TRIM_RATE_MAX`: how fast trim moves at full stick deflection (deg/s)
- `TRIM_EXPO`: curvature — higher = more resolution near center, faster at extremes

For drift nulling: keep `TRIM_RATE_FINE` low (10-20 deg/s).
For post-crash re-acquire: keep `TRIM_RATE_MAX` high (360+ deg/s).
Default expo 0.3 gives a gentle curve. Increase if you want finer center control.

### RPM Hold Governor

Enable: `set RPM_HOLD_ENABLED 1`

- `RPM_HOLD_KP` (default 0.002): P gain — how aggressively throttle adjusts
- `RPM_HOLD_FF` (default 0.5): feedforward — base throttle before correction

The governor holds spin speed against battery sag. No integral term (can't brake).
If RPM oscillates, reduce `RPM_HOLD_KP`. If it tracks too slowly, increase it.

### Spin-Up Slew (`OMEGA_SLEW_MAX`)

Rate-limits omega changes (rad/s²). Prevents the firmware from reacting to sensor
glitches. If the bot seems sluggish on spin-up, increase this. If hits cause omega
readings to spike, decrease it.

## Hit Robustness

### Hit Threshold (`HIT_THRESH_G`)

Accelerometer reading delta that triggers hit detection. Lower = more sensitive.
If false hits trigger on normal driving, increase it.
Default 50g is a starting point — adjust based on your bot's g-forces during
normal operation vs actual impacts.

### Gyro Blanking (`GYRO_BLANK_MS`)

How long to suppress gyro orientation readings after a hit. During this window,
the accel differential keeps tracking heading but orientation state is frozen.

- Too short: gyro sees impact transient → false inversion
- Too long: real inversions take too long to detect
- Default 100ms: good starting point

## Slip Detection

Slip detection requires bidirectional DShot (eRPM telemetry from ESC).

- `SLIP_WARN_PCT` (default 25%): warning threshold
- `SLIP_WARN_MS` (default 300ms): how long slip must sustain before warning

High slip = wheels spinning faster than the bot rotates = poor friction contact.
Check wheel preload, tire condition, contact surface.

## Low Voltage

- `LVC_WARN_VOLTS` (default 3.3V/cell): warning threshold — TX alert + LED overlay
- `LVC_CRIT_VOLTS` (default 3.0V/cell): critical — auto spin-down over 2 seconds
- `CELL_COUNT` (default 0 = auto-detect): override if auto-detect fails

**B4: Auto cell-count ambiguity warning:**
Auto-detection works by measuring battery voltage at boot and dividing by 4.2V:
- 3S (3 cells): 9.0V–12.6V → detected as 3S when 10.5V–12.6V
- 4S (4 cells): 12.0V–16.8V → detected as 4S when 14.0V–16.8V
- **Danger zone: 12.0V–13.0V** — this is both a full 3S AND a dying 4S.
  If you run 4S, set `CELL_COUNT 4` explicitly. Auto-detect only runs at boot.
  Running a 4S pack detected as 3S means LVC fires too late — possible cell damage.
- If auto-detect reads below 9.0V (no battery on USB), cell count = 0 and LVC is disabled.

## LED Configuration

- `LED_COUNT`: number of WS2812 LEDs on the strip
- `LED_ARC_WIDTH`: beacon arc width in degrees (wider = easier to see, less precise)
- Beacon colors configurable for upright (default green) and inverted (default orange)
- All state colors configurable — see `list` for the full set
