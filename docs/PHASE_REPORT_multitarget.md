# MeltyFC Phase Report — Multi-Target Expansion
## Date: 2026-07-09
## Rounds: 5, 6, 6-closeout
## verify.sh: GREEN on all 4 targets on Pi

---

## Build Results (from verify.sh output)

| Target | Status | Flash | RAM | DMA Section |
|--------|--------|-------|-----|-------------|
| crux_f405hd (F405) | SUCCESS | 1,104 / 1,048,576 (0%) | 1,600 / 196,608 (0%) | N/A (default RAM is DMA-safe) |
| aikon_f7mini (F722) | SUCCESS | 3,500 / 524,288 (0%) | 1,600 / 262,144 (0%) | .dtcm_dma: defined, empty (stubs) |
| jhemcu_ghf745 (F745) | SUCCESS | 3,476 / 1,048,576 (0%) | 1,600 / 327,680 (0%) | .dtcm_dma: defined, empty (stubs) |
| micoair_h743v2 (H743) | SUCCESS | 5,448 / 2,097,152 (0%) | 1,600 / 1,048,576 (0%) | .d2_dma: defined, empty (stubs) |

**Native tests:** 245/245 PASSED, ASan/UBSan clean

**Pre-integration skeletons:** These builds validate toolchain, linker scripts, and
map placement only. DMA buffer symbols are `static` in driver files not yet called
from main() — sections are defined but empty. Buffers will populate at integration.
Flash/RAM percentages will grow significantly when real driver code is linked.

---

## Map-Gate Teeth Demo

Demonstrated that removing `.dtcm_dma` from the F7 linker script causes the section
to disappear from the map file entirely. verify.sh catches this:

```
=== TEETH DEMO: .dtcm_dma removed from F7 linker script ===
Section NOT found in map — gate would FAIL
FAIL: aikon_f7mini: .dtcm_dma section MISSING from map AND linker script (I-11a)
=== Restored ===
```

The gate has teeth. A collapsed, renamed, or dropped section cannot pass silently.

---

## Round 5 Findings — Status

| ID | Severity | Description | Status | Commit |
|----|----------|-------------|--------|--------|
| G1 | HIGH | verify.sh single target + hardcoded budgets | FIXED | a9cfc1c |
| G2 | HIGH | HAL driver bodies are stubs | FIXED | d478640 |
| G3 | MED | PORTING.md missing | FIXED | a9cfc1c |
| G4 | MED | Boundary audit not extended | FIXED | a9cfc1c |
| G5 | MED | No phase report | FIXED | this document |
| G6 | LOW | Clock guard, D2 constants, budget parsing | FIXED | a9cfc1c |

## Round 6 Findings — Status

| ID | Severity | Description | Status | Commit |
|----|----------|-------------|--------|--------|
| R6-1 | HIGH | No linker scripts for DMA sections | FIXED | 0a67b47 |
| R6-2 | HIGH | Clock/PWR bodies empty | FIXED | 0a67b47 |
| R6-3 | HIGH | No disarm-frame prefill | FIXED | 0a67b47 |
| R6-4 | MED | WS2812 double-buffer + DMA_NORMAL | AMENDED | 0a67b47, PORTING.md |
| R6-5 | LOW | DMAMUX magic numbers | FIXED | 0a67b47 |
| R6-6 | PROCESS | No phase report | FIXED | this document |

## Round 6 Close-out — Status

| Item | Description | Status | Commit |
|------|-------------|--------|--------|
| 1 | Map-gate fail on missing section | FIXED | this commit |
| 2 | I-12 boot assert | PRESENT (main.cpp:164-179) | 81350b9 |
| 3 | Explicit DMA_NORMAL in F4/F7 | FIXED | this commit |
| 4 | Phase report | FIXED | this document |

---

## Invariants — Complete List

| ID | Rule | Enforcement |
|----|------|------------|
| I-1 | No internal flash writes | verify.sh grep-gate |
| I-2 | No option-byte code | verify.sh grep-gate |
| I-3 | DMA_NORMAL only (non-circular) | Explicit in all 6 driver files |
| I-11 | F4: DMA buffers not in CCM | dma_buf.h assert + runtime hang |
| I-11a | F7: DMA buffers in DTCM | dma_buf.h assert + linker script + map-gate |
| I-11b | H7: DMA buffers in D2 SRAM | dma_buf.h assert + linker script + map-gate + MPU |
| I-12 | Clock assert at boot | main.cpp: SystemCoreClock vs TARGET_CLOCK_MHZ |

---

## Safety Items — Reopened and Closed

| Item | Original | Reopened by | Status |
|------|----------|------------|--------|
| 4c: Uninitialized DMA buffer | Round 4 | R6-3 (new drivers had no prefill) | CLOSED: disarm frame prefill in all families |

---

## WS2812 Double-Buffer Amendment

Formally accepted by reviewer. Single-buffer + busy-check provides correctness
for all pre-POV use cases. Hardware double-buffering deferred to P10 (POV display)
when hardware is available for buffer-swap timing validation. Written rationale
in PORTING.md.

---

## Remaining Hardware-Gated Items

All items below require physical boards to proceed:

1. Step 0 BF dump — diff against BF_CONFIG_DERIVED pinmaps
2. DMA stream/channel resolution from live timer maps
3. GPIO AF verification
4. DMA TC interrupt handler wiring
5. Edge-to-GCR capture-to-frame conversion validation
6. Clock PLL lock verification (I-12 assert will catch failures)
7. MPU region activation on H743 (code present, needs boot validation)
8. WS2812 timing calibration per family (scope check)

---

*Report generated as part of the work, not after it.*
*verify.sh run on Pi (Raspberry Pi 4, ARM64, PlatformIO 6.x)*
