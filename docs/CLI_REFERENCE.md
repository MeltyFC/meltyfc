# MeltyFC CLI Reference

USB CDC serial interface for configuration, diagnostics, and calibration.
Connect via any serial terminal at the default baud rate.

## Commands

### `get <name>`
Read a parameter value.

```
> get MAX_RPM
MAX_RPM = 3200 rpm
```

### `set <name> <value>`
Set a parameter value. Values are clamped to valid range.

```
> set MAX_RPM 2800
> get MAX_RPM
MAX_RPM = 2800 rpm
```

Boolean parameters accept `on`/`off`, `true`/`false`, `0`/`1`:
```
> set RPM_HOLD_ENABLED on
> set BLACKBOX_ENABLED off
```

### `dump`
Dump all parameters as replayable `set` commands (Betaflight pattern).
Copy-paste this output to restore a configuration.

```
> dump
# MeltyFC config dump — schema v1
set NUM_DRIVE_MOTORS 3
set MOTOR_ANGLE_0 0
set MOTOR_ANGLE_1 120
...
```

### `save`
Persist current configuration to internal flash. Survives power cycles.

### `defaults`
Reset all parameters to factory defaults. Does NOT auto-save — use `save` after if you want to persist.

### `list`
List all parameters with type, range, unit, and description.

```
> list
NAME                     TYPE     MIN      MAX      UNIT   DESCRIPTION
----                     ----     ---      ---      ----   -----------
NUM_DRIVE_MOTORS         uint8    1        4               Number of drive motors (1-4)
...
```

### `list json`
Machine-readable JSON array of all parameter definitions. Designed for future configurator GUI auto-generation.

```
> list json
[
  {"name":"NUM_DRIVE_MOTORS","type":"uint8","min":1.0000,"max":4.0000,"default":3.0000,"unit":"","desc":"Number of drive motors (1-4)","readonly":false,"reboot":false},
  ...
]
```

### `status`
Live telemetry output. Displays current state including:
- Spin rate (omega, RPM)
- Phase angle
- Orientation (upright/inverted)
- Slip percentage (per motor)
- Link quality
- Sensor health
- Hit count and max-G
- Battery voltage and cell count

### `cal`
Enter calibration mode. Available calibration procedures:
1. **dr_eff calibration**: Spin at fixed throttle, adjust effective radius difference until reported RPM matches counted LED flashes
2. **Beacon position trim**: Rotate where the beacon arc appears vs true front

Exit calibration mode by disarming.

### `version`
Display firmware version, target board, schema version, and parameter count.

```
> version
MeltyFC v0.1.0-dev
Target: crux_f405hd
Schema: v1
Params: 55
```

### `help`
Display command summary.

## Protocol Notes

- Line endings: CR+LF (`\r\n`) on output, accepts CR, LF, or CRLF on input
- All parameter names are UPPER_CASE with underscores
- Command names are case-insensitive (`GET`, `get`, `Get` all work)
- Parameter names in `get`/`set` are case-sensitive
- Unknown commands return `Error: unknown command`
- USB CDC VCP — same USB port used for flashing
