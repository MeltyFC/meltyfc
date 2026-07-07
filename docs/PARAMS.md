# MeltyFC Parameter Reference

Auto-generated from the parameter registry (`config_store.cpp`).
All parameters are runtime-configurable via USB CDC CLI.

## CLI Commands

| Command | Description |
|---------|-------------|
| `get <name>` | Read parameter value |
| `set <name> <val>` | Set parameter value |
| `dump` | Dump all params (replayable via copy-paste) |
| `save` | Persist config to flash |
| `defaults` | Reset all params to defaults |
| `list` | List all parameters with types/ranges |
| `list json` | Machine-readable JSON for configurator GUI |
| `status` | Live telemetry (omega, RPM, orientation...) |
| `cal` | Enter calibration mode |
| `version` | Firmware version info |
| `help` | Command reference |

## Parameters

| Name | Default | Min | Max | Unit | Description |
|------|---------|-----|-----|------|-------------|
| `NUM_DRIVE_MOTORS` | 3 | 1 | 4 |  | Number of drive motors (1-4) |
| `MOTOR_ANGLE_0` | 0 | 0 | 360 | deg | Motor 0 mounting angle |
| `MOTOR_ANGLE_1` | 120 | 0 | 360 | deg | Motor 1 mounting angle |
| `MOTOR_ANGLE_2` | 240 | 0 | 360 | deg | Motor 2 mounting angle |
| `MOTOR_ANGLE_3` | 0 | 0 | 360 | deg | Motor 3 mounting angle |
| `SPIN_DIRECTION` | 0 | 0 | 1 |  | Spin direction (0=CW, 1=CCW) |
| `MAX_RPM` | 3200 | 100 | 10000 | rpm | Maximum spin RPM (governor cap) |
| `R_INNER` | 15 | 1 | 200 | mm | Inner accel sensor radius |
| `R_OUTER` | 28 | 1 | 200 | mm | Outer accel sensor radius |
| `DR_EFF` | 13 | 0.1 | 200 | mm | Effective radius difference (calibrated) |
| `WHEEL_DIA` | 40 | 5 | 200 | mm | Drive wheel diameter |
| `WHEEL_MOUNT_RADIUS` | 85 | 10 | 500 | mm | Wheel center to spin axis |
| `DRIVE_RATIO` | 5.0 | 0.1 | 100 |  | Gear ratio bell:wheel (1.0=direct) |
| `MOTOR_POLES` | 14 | 2 | 50 |  | Motor pole count (for eRPM conversion) |
| `WINDOW_HALF` | 30 | 5 | 90 | deg | Translation window half-width |
| `THROTTLE_SPIN_MAX` | 0.75 | 0 | 1.0 |  | Max spin throttle (0.0-1.0) |
| `THROTTLE_CAP` | 0.90 | 0 | 1.0 |  | Absolute throttle cap (0.0-1.0) |
| `TRIM_RATE_FINE` | 15 | 1 | 180 | deg/s | Trim rate at min deflection |
| `TRIM_RATE_MAX` | 360 | 10 | 1080 | deg/s | Trim rate at full deflection |
| `TRIM_EXPO` | 0.3 | 0 | 1.0 |  | Trim expo (0=linear, 1=max curve) |
| `ORIENT_DEBOUNCE_MS` | 150 | 10 | 1000 | ms | Orientation change debounce |
| `HIT_THRESH_G` | 50 | 5 | 400 | g | Hit detection threshold |
| `GYRO_BLANK_MS` | 100 | 10 | 1000 | ms | Gyro blanking after hit |
| `OMEGA_SLEW_MAX` | 200 | 10 | 5000 | rad/s2 | Max omega rate of change |
| `SLIP_WARN_PCT` | 25 | 1 | 100 | % | Slip warning threshold |
| `SLIP_WARN_MS` | 300 | 50 | 5000 | ms | Slip warning sustain time |
| `LOWSPEED_SWITCH_RPM` | 900 | 100 | 3000 | rpm | Switch to ICM below this RPM |
| `RPM_HOLD_ENABLED` | 0 | 0 | 1 |  | RPM hold on/off (0=off, 1=on) |
| `RPM_HOLD_KP` | 0.002 | 0.0001 | 0.1 |  | RPM hold P gain |
| `RPM_HOLD_FF` | 0.5 | 0 | 1.0 |  | RPM hold feedforward (base throttle) |
| `CREEP_THRESHOLD_RPM` | 200 | 50 | 1000 | rpm | Enter creep below this RPM |
| `CREEP_HYSTERESIS_RPM` | 50 | 10 | 500 | rpm | Creep exit hysteresis |
| `LED_COUNT` | 12 | 1 | 144 |  | Number of WS2812 LEDs |
| `LED_ARC_WIDTH` | 45 | 5 | 180 | deg | Beacon arc width |
| `CH_TRANSLATE_X` | 0 | 0 | 15 |  | CRSF channel: translate X (0-based) |
| `CH_TRANSLATE_Y` | 1 | 0 | 15 |  | CRSF channel: translate Y (0-based) |
| `CH_SPIN` | 2 | 0 | 15 |  | CRSF channel: spin throttle (0-based) |
| `CH_TRIM` | 3 | 0 | 15 |  | CRSF channel: heading trim (0-based) |
| `CH_ARM` | 4 | 0 | 15 |  | CRSF channel: arm switch (0-based) |
| `CH_RESYNC` | 5 | 0 | 15 |  | CRSF channel: heading re-sync (0-based) |
| `CH_CREEP_FORCE` | 6 | 0 | 15 |  | CRSF channel: force creep mode (0-based) |
| `FAILSAFE_MS` | 500 | 500 | 5000 | ms | Failsafe timeout (hard floor 500ms) |
| `WATCHDOG_MS` | 200 | 50 | 1000 | ms | Watchdog timeout |
| `LVC_WARN_VOLTS` | 3.3 | 2.5 | 4.2 | V | Low voltage warning per-cell |
| `LVC_CRIT_VOLTS` | 3.0 | 2.0 | 4.0 | V | Low voltage critical per-cell |
| `CELL_COUNT` | 0 | 0 | 12 |  | Battery cell count (0=auto) |
| `BLACKBOX_ENABLED` | 1 | 0 | 1 |  | Blackbox logging on/off |
| `BLACKBOX_RATE_HZ` | 100 | 10 | 1000 | Hz | Blackbox log rate |
| `TRANSLATE_DEADBAND` | 0.05 | 0 | 0.5 |  | Translation stick deadband (0.0-1.0) |
| `RESYNC_MIN_DEFLECT` | 0.3 | 0.1 | 1.0 |  | Min stick deflection for re-sync (0.0-1.0) |
| `RESYNC_AVERAGE_MS` | 100 | 10 | 1000 | ms | Stick angle averaging window |

**Total: 51 parameters**

## Notes

- `FAILSAFE_MS` has a hard floor of 500ms (cannot be set lower)
- `CELL_COUNT = 0` enables auto-detection from pack voltage
- Channel indices are 0-based CRSF channel numbers
- All angles in degrees unless noted
- All distances in mm unless noted
