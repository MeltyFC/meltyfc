# MeltyFC — Silicon Errata Audit (L1)
## Status: SEED from training knowledge + Fable prepass
## MANDATORY: Diff against actual ES PDFs before first hardware flash
## ES docs: ES0182 (F405) · ES0287 (F411) · ES0290 (F72x/F74x) · ES0392 (H743) · H72x ES TBD

Each row: erratum ID · affected rev(s) · our exposure · disposition.
Dispositions: AFFECTED->fix | NOT-AFFECTED->rationale | DEFERRED->bench gate

---

## STM32F405/F407 (ES0182)

| ID | Erratum | Our Exposure | Disposition |
|----|---------|-------------|-------------|
| 2.1.1 | I2C analog filter BUSY lockup | H3LIS331 on I2C | **AFFECTED -> FIXED** (E-01). i2c_recovery.hpp step 4.5: RCC peripheral force-reset between retries. Handles both slave-side (9x SCL) and master-side (RCC reset) wedge. |
| 2.1.2 | I2C SMBus alert not cleared | Not using SMBus | NOT-AFFECTED |
| 2.5.x | DMA2 concurrent AHB corruption | All DMA is MEM_TO_PERIPH into APB timers | **NOT-AFFECTED** (E-02). Zero mem-to-mem or GPIO-DMA. Guard: PORTING.md note. |
| 2.5.x | DMA FIFO/burst corruption | FIFOMode=DISABLE explicit everywhere | **NOT-AFFECTED** (E-03). Direct mode only, verified. |
| 2.8.x | TIM triggered-reset glitch | We use PWM mode, not triggered reset | NOT-AFFECTED — verify exact erratum ID against ES |
| 2.8.x | TIM break input + DMA interaction | TIM1/TIM8 break used for MOE | WALK-VERIFY — check if our MOE enable triggers this |
| 2.12.x | USB OTG FS host-mode issues | CDC device only | NOT-AFFECTED — device mode only |
| 2.12.x | USB OTG suspend/resume quirks | CDC only, low traffic | WALK-VERIFY — check if CDC-only is affected |
| 2.14.x | Flash read disturb at high temp | 85°C+ operation possible in bot | WALK-VERIFY — check if our flash access pattern triggers it |
| 2.16.x | IWDG window watchdog interaction | Using IWDG only, no WWDG | NOT-AFFECTED |

## STM32F411 (ES0287)

| ID | Erratum | Our Exposure | Disposition |
|----|---------|-------------|-------------|
| I2C BUSY lockup | Same as F405 | Same | **AFFECTED -> FIXED** (same i2c_recovery module) |
| TIM errata | Subset of F405 | Same patterns | WALK-VERIFY against ES0287 |
| ADC errata | Using polled single-conversion | WALK-VERIFY — check for single-shot errata |
| USB OTG | CDC device, 48MHz via PLLQ | WALK-VERIFY — F411-specific USB errata |
| No TIM8 | F411 lacks TIM8 | Already handled in driver (ifdef TIM8) | N/A |

## STM32F722/F745 (ES0290)

| ID | Erratum | Our Exposure | Disposition |
|----|---------|-------------|-------------|
| F7 cache/DMA coherency | DMA buffers in DTCM (never cached) | **NOT-AFFECTED BY DESIGN** (E-04). I-11a + boot assert + ldscript + map-gate. Three enforcement layers. |
| I2C TIMINGR-based IP errata | Standard master mode, clock-stretch defaults | WALK-VERIFY — check NOSTRETCH corner cases |
| DMA2 stream conflicts | Our DMA allocation TBD per board | WALK-VERIFY at integration |
| USB OTG FS | CDC device | WALK-VERIFY |

## STM32H743 (ES0392)

| ID | Erratum | Our Exposure | Disposition |
|----|---------|-------------|-------------|
| Rev Y 400MHz/no VOS0 | Our config is 400MHz/VOS1 | **NOT-AFFECTED — accidentally revision-proof** (E-05). Made DELIBERATE: bench step reads DBGMCU_IDC/REV_ID. Any future 480MHz experiment requires rev V+ confirmation. |
| DMAMUX overrun/sync flags | DShot DMAMUX driver | WALK-VERIFY — clear flags on configure |
| SDMMC early-rev advisories | Simple single-block writes planned | WALK-VERIFY — verify single-block not affected |
| RAM/ECC per-revision notes | Using AXI + D2 SRAM | **WALK-VERIFY** (MED confidence — recall fuzzy) |
| D-cache/DMA coherency | MPU non-cacheable region over D2 | NOT-AFFECTED — MPU architecture immunity |
| USB OTG SOF/session | CDC device, HSI48+CRS | WALK-VERIFY |

## STM32H723/H725 (ES TBD — newer part, thin recall)

| ID | Erratum | Our Exposure | Disposition |
|----|---------|-------------|-------------|
| SMPS transition/timing | Board uses SMPS (BetaFPV H725) | **WALK-VERIFY** — flag specifically before any 520MHz/VOS0 experiment |
| VOS0 region constraints | Currently at 400MHz/VOS1 (safe) | NOT-AFFECTED (parked at H743-proven chain) |
| All other H72x errata | Newer part, less well-known | **FULL ES WALK REQUIRED** — this part owns nearly wholesale |

## Cross-Cutting (non-silicon)

| Item | Our Exposure | Disposition |
|------|-------------|-------------|
| IWDG hardware-start option byte | Must be software-start | **ADDRESSED** in PROVISIONING.md: "verify IWDG_SW = software start" per board |
| HAL library bugs | Using pinned platform (P-01) | MITIGATED — ststm32@19.7.0 pinned, HAL updates are reviewed changes |

---

## VERIFICATION STATUS

- **E-01 through E-05**: Confirmed from Fable prepass, code-verified
- **WALK-VERIFY items**: Require actual ES PDF text to close. Trip to provide PDFs or Nexus to attempt download on a machine with browser access.
- **H72x**: Thin recall on newer part — full ES walk is mandatory before first H725 flash

## EPISTEMIC HONESTY

This audit is seeded from training-data recall of the ES documents, NOT from reading
the current PDFs. It is the FLOOR of the errata audit, never its ceiling. Every
disposition tagged WALK-VERIFY must be confirmed against the actual erratum text before
the board powers motors. Fable reviews the dispositions against the ES docs when available.
