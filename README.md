# MeltyFC

Open-source meltybrain firmware platform for STM32 flight controllers.

MeltyFC implements a translational-drift ("melty brain") controller with a three-tier
sensor architecture, generic N-motor output, inversion handling, hit-robustness, slip
detection, and CRSF/ELRS integration.

## Status

**Phase 0** — Project skeleton, interfaces, pure-logic modules with tests.
Hardware bringup pending (Step 0 dump required).

## Supported Targets

| Target | Board | MCU | Status |
|--------|-------|-----|--------|
| `crux_f405hd` | Happymodel CruxF405HD ELRS AIO | STM32F405RGT6 | In development |

## Quickstart

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or IDE)
- USB cable for flashing (DFU mode)

### Build

```bash
# Build firmware
pio run -e crux_f405hd

# Run unit tests (no hardware needed)
pio test -e native

# Flash via DFU
pio run -e crux_f405hd -t upload
```

### IMPORTANT: Before Erasing Stock Firmware

**You MUST capture the Betaflight CLI dump before flashing MeltyFC.**
This is the only way to get the correct pin assignments for your board.

1. Connect to Betaflight Configurator
2. CLI tab: `dump all` → save as `docs/PINMAP_source_dump.txt`
3. CLI tab: `resource list` → append to the same file
4. CLI tab: `version` → append

To restore Betaflight later, flash the appropriate `.hex` from the
[Betaflight releases](https://github.com/betaflight/betaflight/releases).

## Architecture

MeltyFC separates hardware from logic via clean interfaces:

- **Core logic** (`src/heading.cpp`, `src/slip.cpp`, `src/safety.cpp`) is pure
  computation — no hardware dependencies, fully unit-testable on any platform.
- **Hardware drivers** implement interfaces defined in `include/interfaces.h`.
- **Board targets** (`targets/`) contain ALL board-specific pin maps and bindings.

Adding a new board requires only a new `targets/` directory — zero core edits.

## Configuration

All bot-specific values are runtime-configurable via USB serial CLI:

```
# Connect via serial terminal (115200 baud)
help            # Show all commands
list            # List all parameters with types/ranges
get WINDOW_HALF # Read a parameter
set WINDOW_HALF 35  # Change a parameter
save            # Persist to flash
dump            # Export all params (diff-able, re-playable)
status          # Live telemetry (omega, RPM, orientation, slip%)
```

See [PARAMS.md](docs/PARAMS.md) and [CLI_REFERENCE.md](docs/CLI_REFERENCE.md)
for the full parameter registry and command reference.

## License

GPLv3 — see [LICENSE](LICENSE).

## Acknowledgments

MeltyFC is an independent implementation inspired by the meltybrain concepts
demonstrated in [OpenMelt2](https://github.com/nothinglabs/openmelt2) by Rich Olson.
No code from OpenMelt2 is used in this project. The translational-drift algorithms
are derived from rotational physics (centrifugal force, phase-locked motor timing).
