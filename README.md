# MeltyFC

**Open-source meltybrain firmware for combat robot flight controllers.**

Turn any supported FPV flight controller into a translational-drift ("melty brain") spin controller. MeltyFC handles heading tracking, motor timing, hit recovery, slip detection, and full invertibility — so you can focus on building your bot, not writing firmware from scratch.

Built for the combat robot community. GPLv3.

---

## What Is This?

A meltybrain is a full-body spinner that translates (drives in a direction) by pulsing individual motors at the right point in each rotation. MeltyFC does the math: it tracks heading via dual high-g accelerometers, times motor output windows, handles hits, detects wheel slip, and manages safety — all at 2kHz on a $15 flight controller you probably already have in a drawer.

**You don't need a custom PCB.** MeltyFC runs on off-the-shelf FPV flight controllers with onboard ESCs, gyros, and radio receivers. Flash it, wire two accelerometer breakouts, tune a few parameters, and you have a working meltybrain.

## Supported Hardware

| Target | Board | MCU | Flash | Tier | Status |
|--------|-------|-----|-------|------|--------|
| `crux_f405hd` | Happymodel CruxF405HD | STM32F405 | 1MB | Full | Ready for hardware |
| `betafpv_f411` | BetaFPV F411 AIO | STM32F411 | 512KB | Standard | Ready for hardware |
| `aikon_f7mini` | Aikon F7 Mini 45A | STM32F722 | 512KB | Standard | Ready for hardware |
| `jhemcu_ghf745` | JHEMCU GHF745 | STM32F745 | 1MB | Full | Ready for hardware |
| `micoair_h743v2` | MicoAir H743 V2 | STM32H743 | 2MB | Full | Ready for hardware |
| `betafpv_h725` | BetaFPV H725 AIO | STM32H725 | 1MB | Full | Ready for hardware |

**Don't see your board?** Adding one takes about an hour — see [Adding Your Own Board](#adding-your-own-board) below.

### Feature Tiers

Not all boards have enough flash for every feature. MeltyFC automatically scales:

- **Full** (1MB+): Everything — POV display, full blackbox logging, extended CLI
- **Standard** (512KB): Full control and safety, reduced POV fonts, smaller blackbox ring
- **Minimum** (256KB): Full control and ALL safety features, beacon LEDs only, no POV/blackbox

Safety code is **never** tier-gated. Every board gets the full safety stack regardless of flash size.

## What You Need

### Hardware
- A supported flight controller (or [add your own](#adding-your-own-board))
- 2x Adafruit H3LIS331 breakouts (~$13 each) — the high-g accelerometers for heading
- An ExpressLRS receiver (or any CRSF-compatible radio)
- A 2S-6S LiPo battery (voltage depends on your motors — use the [calculator](https://melty-calc.vercel.app))

### Software
- [PlatformIO](https://platformio.org/) CLI or VS Code extension
- A USB cable for flashing (DFU mode)

## Quick Start

### 1. Capture Your Board's Pin Map (before erasing Betaflight!)

```bash
# Connect to Betaflight Configurator, open CLI tab:
dump all          # Save the output
resource list     # Save this too
version           # And this
```

Save these — they're your board's pin assignments. You'll need them to verify or create a MeltyFC target.

### 2. Build and Flash

```bash
# Clone the repo
git clone https://github.com/MeltyFC/firmware.git
cd firmware

# Run the test suite (no hardware needed — verifies your toolchain)
pio test -e native

# Build for your board
pio run -e crux_f405hd    # Replace with your target

# Flash via DFU (hold boot button, connect USB)
pio run -e crux_f405hd -t upload
```

### 3. Configure Your Bot

Connect via USB serial (115200 baud) and use the CLI:

```
help                    # Show all commands
list                    # List all parameters with types and ranges
get WINDOW_HALF         # Read a parameter
set WINDOW_HALF 35      # Change a parameter
set R_OUTER 0.028       # Outer accelerometer radius in meters
set R_INNER 0.015       # Inner accelerometer radius
set NUM_DRIVE_MOTORS 3  # Number of drive motors
save                    # Persist to flash (A/B redundant storage)
dump                    # Export all params (diff-able, replayable)
```

### 4. Bench Test

**Read [BENCH_TEST.md](docs/BENCH_TEST.md) before spinning anything.** It walks you through every safety check — failsafe timing, arm gating, voltage calibration, and the critical TX-off test. Skip nothing.

## Architecture

```
src/
  heading.cpp/hpp     # Heading engine: differential accel -> omega -> phase -> motor windows
  safety.cpp/hpp      # Arming state machine, failsafe, choke point (ALL motor authority here)
  slip.cpp/hpp        # Per-motor slip detection via bidirectional DShot eRPM
  lvc.cpp/hpp         # Low voltage cutoff with per-cell thresholds and spin-down ramp
  dshot_common.cpp    # DShot300 frame packing, CRC, GCR decode (pure logic, no hardware)
  config_cli.cpp      # USB CLI with transactional validation
  config_flash.cpp    # A/B redundant config storage with CRC + sequence bytes
  vbat_filter.hpp     # Battery voltage EMA filter with arming grace window
  ...

src/hal/
  common/             # HAL contracts (dshot_hal.h, ws2812_hal.h, vbat_hal.h, flash_hal.h)
  f4/                 # STM32F4 family drivers (F405, F411)
  f7/                 # STM32F7 family drivers (F722, F745)
  h7/                 # STM32H7 family drivers (H743, H725)

targets/
  crux_f405hd/        # Per-board: pinmap.h, target.h, clock_config.c, linker script
  betafpv_f411/
  ...

test/                 # 256 native unit tests — runs on your dev machine, no hardware
```

**The key idea:** all control logic is pure computation with zero hardware dependencies. The 256-test native suite gives you full coverage of heading math, safety state machine, slip detection, config validation, and protocol handling — before you ever touch a board. Hardware drivers are thin wrappers that implement the HAL contracts.

## Adding Your Own Board

MeltyFC is designed to make new boards easy. You need four files:

### 1. Get your board's Betaflight config

Find it in the [Betaflight unified config repo](https://github.com/betaflight/config) or capture it from your board's CLI. Save it to `docs/pinmap_sources/`.

### 2. Create a target directory

```
targets/your_board/
  pinmap.h          # Pin assignments from the BF config
  target.h          # Board metadata, clock speed, feature tier
  clock_config.c    # PLL configuration for your MCU
  STM32xxxx_MELTY.ld  # Linker script (copy from a same-family board, adjust flash/RAM sizes)
```

### 3. Tag every pin as `BF_CONFIG_DERIVED`

Until you've verified against a live `dump all`, every pin is provisional. The build system warns you about unverified pinmaps.

### 4. Add to `platformio.ini` and `verify.sh`

```ini
[env:your_board]
extends = common_stm32
board = nucleo_xxxx          # Nearest Nucleo board
board_build.mcu = stm32xxxxxx
board_build.ldscript = targets/your_board/STM32xxxx_MELTY.ld
build_flags =
    ${common_stm32.build_flags}
    -D TARGET_YOUR_BOARD
    -D STM32Fxxx             # Your chip family define
    -I targets/your_board
build_src_filter =
    +<*>
    +<../targets/your_board/>
    +<hal/common/>
    +<hal/f4/>               # Your family's HAL
    -<hal/f7/>
    -<hal/h7/>
    -<../test/>
```

Add flash/RAM budgets to the arrays in `verify.sh`, then run `pio run -e your_board` — if it builds, you're most of the way there.

See [PORTING.md](docs/PORTING.md) for family-specific gotchas (DMA placement, timer allocation, clock trees, power supply configuration).

## Safety

MeltyFC takes safety seriously. Combat robots spin fast and can cause injury.

- **Choke point architecture:** ALL motor authority flows through one function. No motor can spin without passing the full safety check.
- **Non-circular DMA:** If the CPU dies, DMA stops, ESCs receive no more frames, motors stop. The ESC's own signal timeout is the final backstop.
- **Fail-closed LVC:** Unknown battery state = motors disabled. Not "probably fine" — disabled.
- **Bidirectional DShot slip detection:** If a wheel is torn off, the firmware detects the free-spinning motor and kills it.
- **12 machine-enforced invariants** verified at build time and boot time
- **9 rounds of adversarial security review** (13 bug-class sweeps)
- **256 native tests** with AddressSanitizer and UndefinedBehaviorSanitizer

Read [VERIFICATION.md](docs/VERIFICATION.md) for the full invariant table and review history.

## Calculator

Plan your bot's spin speed, energy storage, motor selection, and balance:

**[melty-calc.vercel.app](https://melty-calc.vercel.app)**

Interactive friction-drive calculator with donut/puck modes, geometry visualization, KV windows, balance check, and sleeve tuning.

## Documentation

| Document | What it covers |
|----------|---------------|
| [BENCH_TEST.md](docs/BENCH_TEST.md) | First-flash procedure and safety checks (read this before powering anything) |
| [PORTING.md](docs/PORTING.md) | Family differences, DMA rules, adding new chips |
| [VERIFICATION.md](docs/VERIFICATION.md) | Invariant table, review history, integration rules |
| [RECOVERY.md](docs/RECOVERY.md) | DFU recovery when things go wrong |
| [TUNING.md](docs/TUNING.md) | Parameter tuning guide |
| [TODO.md](docs/TODO.md) | Open items and completed work |

## License

GPLv3 — see [LICENSE](LICENSE).

## Acknowledgments

MeltyFC is an independent implementation inspired by the meltybrain concepts demonstrated in [OpenMelt2](https://github.com/nothinglabs/openmelt2) by Rich Olson. No code from OpenMelt2 is used in this project (OpenMelt2 is CC BY-NC-SA, incompatible with GPL). The translational-drift algorithms are derived from rotational physics first principles.

Spec and adversarial review by [Fable](https://www.anthropic.com/) (Anthropic's research model). Implementation by [Nexus](https://github.com/tripartist1) (Claude-based autonomous agent).
