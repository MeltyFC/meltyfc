# MeltyFC — MULTI-TARGET EXPANSION SPEC (F7 family + H743)
### Update document for the implementation agent (Opus via Nexus) · 2026-07-08
### Commit to repo as docs/MULTI_TARGET_SPEC.md. Extends Melty_Firmware_Spec_v1 §2/§2B;
### all v1 rulings, invariants (I-1..I-10), and VERIFICATION.md remain binding.

## 0. SCOPE & WHY NOW

Add chip-family support for the FC-shortlist boards while hardware is in transit:
- **F7 family:** STM32F722 (Aikon F7 Mini 45A AM32) + STM32F745 (JHEMCU GHF745 AIO).
  One HAL family, two chip variants.
- **H743:** STM32H743 (MicoAir H743 V2 AIO 45A — current rubric leader).
- **G4 explicitly OUT OF SCOPE** — no shortlist board exists; don't build targets at
  nothing.
Everything here is self-verifiable on the Pi (compiles, native tests, sim, budgets);
hardware bench gates are listed but deferred to board arrival. Pure-logic core
(heading, windows, safety, slip, CLI, config) is untouched by definition — this work
is HAL + targets + build system only.

## T0 — PINMAPS WITHOUT HARDWARE (the unblock)

We own none of these boards yet, so there is no live Betaflight dump. **The Betaflight
unified-target config repo (github.com/betaflight/config) is the authoritative
substitute:** per-board target files define motor pins + timers, LED pin, UART/I2C/SPI
assignments, gyro CS, oscillator frequency, and (critically for H743) **the board's
power config.** Tasks:
1. Fetch the unified configs for: Aikon F7 Mini (AIKONF7 or nearest), JHEMCU GHF745,
   MicoAir743 (a Betaflight target exists for MicoAir H743 boards — also check
   MicoAir's own docs/ArduPilot hwdef, they publish full pin tables).
2. Derive `targets/aikon_f7mini/`, `targets/jhemcu_ghf745/`, `targets/micoair_h743v2/`
   pinmaps from them. **Mark every pin `BF_CONFIG_DERIVED — verify on live dump at
   arrival`** — same discipline as the Crux PLACEHOLDER rule, one grade stronger than
   guessing, one grade weaker than a dump. Step 0 on real hardware still happens and
   diffs against these.
3. Commit the fetched config files to `docs/pinmap_sources/` for provenance.

## T1 — F7 FAMILY HAL (the moderate port)

**What's the same as F405 (why this is moderate):** DMA1/DMA2 stream+channel
architecture is F4-like — the bidir-DShot and WS2812 timer/DMA drivers port with
pin/AF/stream-table changes, logic intact. IWDG identical. USB CDC via STM32duino core
(F722/F745 generic variants exist in the core; PlatformIO `genericSTM32F722xx` /
`F745xx` envs).

**What's different (each gets a ruling):**
1. **F7 has L1 I/D caches → the DMA-coherency problem exists here too (milder than
   H7). RULING: all peripheral-DMA buffers live in DTCM (64KB at 0x20000000)** — DTCM
   is never cached AND is DMA-accessible on F7 (via the AHBS slave port). This solves
   coherency for free, no cache-maintenance calls in hot paths. Section attribute
   `.dtcm_dma` + linker fragment + **boot assert: every DMA buffer address in
   [0x20000000, 0x2000FFFF]** (mirror of the F4 CCM assert, inverted logic — new
   invariant I-11a).
2. **I2C peripheral is the new IP (TIMINGR-based), not F4's CCR/TRISE.** We ride
   Arduino Wire, which abstracts it — no driver work — but note it in PORTING.md so
   nobody writes LL I2C assuming F4 registers.
3. **Clock config per chip:** 216MHz for both F722/F745; new `clock_config_f7.c` per
   target using the BF-config oscillator value.
4. **Budgets:** F722 = 512KB flash / 256KB RAM; F745 = 1MB / 320KB (TCM+SRAM split —
   budget gate counts total but the linker must place correctly). Update verify.sh
   per-target gates.
5. Fault handlers: same SCB/CFSR registers on M7 — existing fault_handler logic ports;
   add cache-disable to the fault path before breadcrumb writes (paranoia, cheap).

**Order:** F722 first (Aikon), bidir-DShot GCR spike FIRST on it (same P2 discipline —
it's the hardest driver and the F7 DMA tables are the only real delta). F745 =
pin/flash variant of the same family once F722 compiles green.

## T2 — H743 HAL (the hard port; every trap has a ruling)

1. **Memory domains + DMA reachability (THE trap):** DTCM (0x20000000) is CPU-private —
   **DMA1/DMA2 CANNOT reach it** (inverse of F7!). BDMA only reaches D3/SRAM4. RULING:
   **all peripheral-DMA buffers in D2 SRAM (0x30000000)**, section `.d2_dma`, boot
   assert range (invariant I-11b). MDMA unused in v1.
2. **D-cache vs DMA: RULING = MPU non-cacheable region over the `.d2_dma` section**
   (deterministic; no clean/invalidate calls inside the bidir-DShot 30µs turnaround
   path — per-transfer cache maintenance is explicitly REJECTED for hot paths). MPU
   config in target init, one region, documented.
3. **DMAMUX:** request routing is programmable (this is the *nice* difference) — the
   DShot/WS2812 drivers gain a small DMAMUX-assignment layer in the h7 family dir.
4. **Power config LDO vs SMPS — the won't-boot trap:** H743 boards wire the internal
   regulator differently; wrong `PWR` supply config = chip never reaches main() (looks
   dead; DFU-recoverable, not bricked, but demoralizing). **Take the setting from the
   board's BF/hwdef config (T0), never guess.** MicoAir743's target defines it.
5. **Clock: bring up at 400MHz / VOS1 first** — 480MHz needs VOS0 and buys nothing a
   2kHz superloop needs yet. 480 is a later one-line experiment, not a bring-up risk.
6. **I2C = new IP** (same note as F7). **SDMMC/TF card (MicoAir has a slot):**
   `IFlashStorage` gains a backend enum — SPI-NOR (crux) vs **SDMMC raw-block ring**
   for blackbox on H743 targets. Raw sector ring, CLI-dumpable, NO FatFs dependency in
   v1 (FatFs = optional later; a filesystem is not required to log).
7. Budgets: 2MB flash / ~1MB RAM across domains — verify.sh gates updated; linker
   script per target places .data/.bss in AXI SRAM, DMA in D2, stack in DTCM (fast).
8. Option bytes: **invariant I-2 re-asserted verbatim** — the grep-gate already
   catches it; H7 dual-bank changes nothing about "application never writes internal
   flash."

## T3 — BUILD SYSTEM, STRUCTURE, VERIFICATION

1. **HAL family layout (refactor, small):**
```
src/hal/common/   # dshot_hal.h, ws2812_hal.h, dma_buf.h — the contracts
src/hal/f4/       # existing crux drivers move here
src/hal/f7/
src/hal/h7/
```
   Core still includes ONLY interfaces; targets select a family. **Boundary audit
   extends:** core modules may not include `hal/`; `hal/<family>/` may not include
   another family. Grep-gates in verify.sh.
2. **verify.sh builds ALL targets** (`crux_f405hd`, `aikon_f7mini`, `jhemcu_ghf745`,
   `micoair_h743v2`) with per-target flash/RAM gates. Native tests + sim unchanged —
   they never compile HAL (that's the whole architecture paying off: 245+ tests give
   identical coverage to every family for free).
3. **New invariant I-11 (DMA buffer placement, per family):** F4 = not-CCM (exists);
   F7 = DTCM-only; H7 = D2-only + MPU region active. Boot asserts, all three.
4. **docs/PORTING.md:** capture every family difference learned (I2C IP, cache
   strategy, DMAMUX, PWR config, memory maps) — this file is what makes target #5
   an afternoon.
5. **Phase reports per VERIFICATION.md format**, one per family. Self-verification =
   chain green + all targets compile + budgets pass + boundary clean. **Hardware
   gates (deferred, listed now):** per-board — DShot electrical timing on real AM32,
   bidir turnaround scope check, WS2812 on the derived pin, I2C/SPI sensors, USB,
   PWR-config boot (H743), pinmap diff vs live dump at arrival.

## ORDER & DONE

T0 (configs + derived pinmaps) → T1 F722 (bidir spike first) → T1b F745 variant →
T2 H743 → T3 continuously. **Done =** verify.sh green across 4 targets, I-11 asserts
in all families, PORTING.md written, phase reports posted, every derived pin tagged
for arrival-diff. Fable review rounds (same adversarial protocol as Rounds 1–4) run
after Opus reports each family complete.
