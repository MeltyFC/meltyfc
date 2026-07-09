# MeltyFC — Porting Guide

Every family difference learned during the multi-target expansion, captured
so target #5 is an afternoon.

## Memory Architecture & DMA

| Property | F4 (F405) | F7 (F722/F745) | H7 (H743) |
|----------|-----------|----------------|-----------|
| DMA-safe RAM | Main SRAM | DTCM (0x20000000) | D2 SRAM (0x30000000) |
| DMA-forbidden | CCM (0x10000000) | Non-DTCM | DTCM (CPU-private!) |
| Cache concern | None | L1 mild (DTCM avoids it) | D-cache serious |
| Cache strategy | N/A | DTCM = never cached | MPU non-cacheable region |
| Section attr | none needed | `.dtcm_dma` | `.d2_dma` |
| Assert macro | `DMA_BUFFER_ASSERT` (ban CCM) | `DMA_BUFFER_ASSERT` (require DTCM) | `DMA_BUFFER_ASSERT` (require D2) |
| Invariant | I-11 | I-11a | I-11b |

**The F7→H7 inversion is THE trap:** DTCM is the safe zone on F7 and the
forbidden zone on H7. The same address range (0x20000000) that you want on
F7 is CPU-private on H7. `dma_buf.h` handles this automatically via the
family defines.

## DMA Request Routing

| Family | Mechanism | Implication |
|--------|-----------|-------------|
| F4 | Fixed stream+channel table | Look up in reference manual, no flexibility |
| F7 | Fixed stream+channel table | Same as F4, different assignments |
| H7 | DMAMUX (programmable) | Small assignment layer in hal/h7/, much more flexible |

## I2C Peripheral

| Family | IP | Configuration |
|--------|-------|--------------|
| F4 | Legacy (CCR/TRISE based) | Manual timing calculation from APB clock |
| F7 | New IP (TIMINGR based) | Different register set, different timing calc |
| H7 | Same as F7 (TIMINGR) | Same new IP |

**Arduino Wire abstracts this.** No driver work needed. But if anyone writes
LL I2C code, they MUST NOT assume F4 registers on F7/H7.

## Power Configuration

| Family | Concern |
|--------|---------|
| F4 | None — straightforward |
| F7 | None — straightforward |
| H7 | **CRITICAL: LDO vs SMPS.** Wrong `PWR_ConfigSupply()` = chip never boots. Get the setting from the board's BF config or hwdef. H743 = LDO (no SMPS). DFU-recoverable but demoralizing. |

## Clock Configuration

| Target | HSE | SYSCLK | Approach |
|--------|-----|--------|----------|
| CruxF405HD | 8MHz | 168MHz | Standard F4 PLL |
| Aikon F7 Mini | 8MHz | 216MHz | F7 PLL |
| JHEMCU GHF745 | 8MHz | 216MHz | F7 PLL (same family) |
| MicoAir H743 V2 | 8MHz | 400MHz (VOS1) | Safe bring-up. 480MHz (VOS0) is one-line later. |

HSE values come from the BF unified config files saved in `docs/pinmap_sources/`.

## Cache Management Rules

1. **F4:** No cache. No concern.
2. **F7:** L1 I/D cache exists. DMA buffers in DTCM avoid it entirely (DTCM is never cached on F7).
3. **H7:** D-cache is real.
   - **MPU non-cacheable region** over the `.d2_dma` section = the ONLY acceptable solution.
   - **Per-transfer cache maintenance (clean/invalidate) is REJECTED** for hot paths (bidir DShot 30us turnaround).
   - MPU configured once at boot in `system_h7.cpp`, never touched again.
   - Cache-disable added to fault handler path before breadcrumb writes (paranoia, cheap).

## Fault Handling

Same SCB/CFSR registers on all Cortex-M4/M7 cores. Existing `fault_handler` logic
ports unchanged. On F7/H7: disable cache before breadcrumb writes in fault path.

## Adding a New Target

1. Fetch the BF unified config from `github.com/betaflight/config`
2. Cross-reference with ArduPilot hwdef if available
3. Create `targets/<board>/pinmap.h` — tag all pins `BF_CONFIG_DERIVED`
4. Create `targets/<board>/target.h` — board metadata, feature flags
5. Create `targets/<board>/clock_config.c` — PLL tree from BF/hwdef
6. Save source configs to `docs/pinmap_sources/`
7. Determine which HAL family (f4/f7/h7) — or create a new one if needed
8. Add env to `platformio.ini` with correct board, MCU override, family defines
9. Add target to `ALL_TARGETS` array in `verify.sh` with flash/RAM budgets
10. Run `verify.sh` — all targets must build, all tests must pass

### PlatformIO Gotchas
- Generic board IDs often don't exist — use Nucleo + `board_build.mcu` override
- Family defines (`STM32F7xx`) are NOT auto-set — add to `build_flags`
- `build_src_filter` must include `hal/<family>/` and exclude other families

## WS2812 Double-Buffer Amendment

The v1 spec (§3) mandated DMA + double-buffer for WS2812. The current
implementation uses **single-buffer with busy-check** (ws2812Busy() gates
new writes). This is an explicit amendment, not an oversight:

1. Hardware DMA double-buffering (DMA_DOUBLE_BUFFER_M0) requires the DMA
   stream to swap buffer pointers on transfer-complete — complex to set up
   and test without hardware.
2. Single-buffer with busy-check prevents render-during-transfer (the
   actual failure mode double-buffering prevents).
3. POV display (the use case requiring seamless updates) is the last v1
   feature (P10). Double-buffer will be implemented during POV work when
   we have hardware to validate the buffer swap timing.
4. For non-POV LED states (beacon, status, arm indicator), single-buffer
   at ~30 FPS is more than sufficient.

**Status: deferred to P10 (POV). Single-buffer busy-check provides
correctness guarantee for all pre-POV use cases.**
