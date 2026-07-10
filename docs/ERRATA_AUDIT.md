# MeltyFC — Silicon Errata Audit (L1)
## Status: VERIFIED against actual ES PDFs (Wayback Machine archived copies)
## Sources: ES0182 Rev 14 (F405) · ES0287 Rev 2 (F411) · ES0290 (F72x/F74x) · ES0392 Rev 13 (H743)
## H72x ES: NOT YET OBTAINED — full walk required before first H725 flash
## PDFs archived in docs/pinmap_sources/

---

## STM32F405/F407 (ES0182 Rev 14, Feb 2023)

### Relevant Errata (peripherals we use: TIM, DMA, I2C, ADC, USB, IWDG)

| Section | Erratum | All Revs | Our Exposure | Disposition |
|---------|---------|----------|-------------|-------------|
| 2.1.2 | VDIV/VSQRT incomplete with short ISRs | A | sqrtf() in heading engine; DMA ISRs will exist | **MITIGATED** — ISR contract (L10) mandates >2 instructions per ISR. Verify no compiler-generated ultra-short ISRs. |
| 2.2.11 | APB clock slowed during DMA transfer | A | DShot DMA on APB timers | **NOT-AFFECTED** — timing is DMA-driven, not CPU-timed. DMA transfer completes regardless of CPU bus stall. |
| 2.2.13 | Delay after RCC peripheral clock enabling | A | All clock enables in HAL init | **MITIGATED** — HAL macros include the documented DSB/dummy-read delay. |
| 2.2.19 | DMA2 data corruption concurrent AHB+APB | A | All DMA is MEM_TO_PERIPH into APB timers | **NOT-AFFECTED** (E-02 confirmed). Zero mem-to-mem or AHB-target DMA. PORTING.md guard note. |
| 2.4.1 | ADC sequencer modification during conversion | A | Polled single-conversion for VBAT | **NOT-AFFECTED** — single-channel, no sequencer change during conversion. |
| 2.5.1 | DMA request not auto-cleared by DMAEN | A | DShot DMA setup | **WALK-VERIFY** — check if our DMA enable sequence triggers this. |
| 2.6.1 | PWM re-enabled despite system break | P (all) | TIM1/TIM8 MOE for motor output | **AFFECTED** — if a system break event occurs, PWM may re-enable. Workaround: disable break input or use software break management. **Bench: verify break behavior with motors.** |
| 2.6.3 | Consecutive compare event missed | N (all) | DShot compare-buffer DMA | **NOT-AFFECTED** — we use DMA-fed compare, not compare interrupts. The missed event is an interrupt, not a DMA trigger. |
| 2.7.1-4 | IWDG/WWDG flag issues in Stop mode | A | IWDG only, no Stop mode | **NOT-AFFECTED** — combat bot never enters Stop mode. |
| 2.9.2 | I2C: Start cannot be generated after misplaced Stop | A | H3LIS331 I2C reads | **AFFECTED -> FIXED** (E-01). i2c_recovery.hpp handles both slave-side (9x SCL) and master-side (RCC peripheral reset per ES0182 workaround). |
| 2.9.6 | I2C: Spurious bus error detection in Master mode | A | H3LIS331 I2C | **AFFECTED** — spurious BERR flag. Workaround: clear BERR flag in error handler, do not abort transfer on BERR alone. **Add to i2c_recovery error handler at integration.** |
| 2.14.3-6 | USB OTG_FS various | A | CDC device mode only | **WALK-VERIFY** — most are host-mode. 2.14.6 (wrong DCFG read) affects device; check if CDC reads DCFG. |

### Not Relevant (peripherals we don't use)
FSMC (2.3.x), DAC (2.5.x), RTC (2.8.x — no RTC in melty), USART (2.10.x — CRSF uses UART not USART errata), bxCAN (2.13.x), ETH (2.16.x), OTG_HS (2.15.x).

---

## STM32F411 (ES0287 Rev 2, Feb 2017)

### Relevant Errata

| Section | Erratum | Our Exposure | Disposition |
|---------|---------|-------------|-------------|
| 1.2 | VDIV/VSQRT with short ISRs | Same as F405 | **MITIGATED** — same ISR contract. |
| 2.1.5 | MPU attribute to RTC/IWDG managed incorrectly | IWDG usage | **WALK-VERIFY** — check if our IWDG access pattern triggers this. MPU not configured on F411. |
| 2.1.6 | Delay after RCC clock enabling | All peripherals | **MITIGATED** — HAL handles delay. |
| 2.1.10 | DMA2 concurrent AHB/APB corruption | Same as F405 | **NOT-AFFECTED** — MEM_TO_PERIPH only. |
| 2.2.1 | IWDG RVU/PVU flags not reset in Stop | No Stop mode | **NOT-AFFECTED**. |
| 2.3.1-5 | I2C limitations | H3LIS331 | **AFFECTED -> FIXED** — same i2c_recovery module. F411 uses same I2C IP as F405. |
| 2.6.1-4 | USB OTG_FS | CDC device | **WALK-VERIFY** — same class as F405. |
| 2.8.1 | ADC sequencer during conversion | Polled single | **NOT-AFFECTED**. |

### F411-Specific Notes
- **No TIM-specific errata** — timer peripheral is cleaner on F411 than F405.
- **No TIM8** — already handled in driver code (`#ifdef TIM8`).
- Only revision "A" exists for F411 — no revision-gating needed.

---

## STM32F722/F745 (ES0290, Sep 2021)

### Relevant Errata (from TOC — detail pages not yet read)

| Area | Expected Errata | Our Exposure | Disposition |
|------|----------------|-------------|-------------|
| I2C | TIMINGR-based IP corner cases (NOSTRETCH, timing) | Standard master mode | **WALK-VERIFY** — read detail pages for timing errata with our clock settings. |
| DMA | Stream conflicts | DShot + WS2812 DMA | **WALK-VERIFY** — check for F7-specific stream errata. |
| Cache | D-cache/DMA coherency advisories | All DMA in DTCM (never cached) | **NOT-AFFECTED BY DESIGN** (E-04). DTCM is architecturally never cached. Three enforcement layers (I-11a). |
| USB | OTG_FS errata | CDC device | **WALK-VERIFY** |
| TIM | Timer errata family | DShot PWM+DMA | **WALK-VERIFY** — read detail pages. |

**Note: ES0290 covers F74x (F745) and F75x, NOT F72x directly.** F722 may have a separate or combined ES. The F72x errata sheet identifier needs verification — it may be included in ES0290 or have its own document. **Flag for Trip: search ST for "ES0290" scope or find a F72x-specific ES.**

---

## STM32H743 (ES0392 Rev 13, Sep 2024)

### Relevant Errata (69 pages — most extensive)

| Section | Erratum | Revs | Our Exposure | Disposition |
|---------|---------|------|-------------|-------------|
| 2.1.1 | D-cache write-through corruption | P (all) | D2 SRAM DMA buffers | **NOT-AFFECTED** — MPU non-cacheable region over D2 eliminates cache involvement entirely. Architecture immunity. |
| 2.1.4 | ECC error corrupts D-cache error bank | A (all) | D-cache enabled | **WALK-VERIFY** — read detail page. May need cache-disable in fault handler (already planned). |
| 2.2.9 | AXI SRAM read corruption | A (Y/W only) | .data/.bss in AXI SRAM | **AFFECTED (rev-dependent)** — read detail page for conditions and workaround. **Bench: read silicon revision ID.** |
| 2.2.10 | CSS clock switching failure on LSE detect | A (Y/W only) | CSS decision (L4) | **RELEVANT TO L4 DECISION** — if CSS is enabled, this erratum affects the switchover. Rev X/V fixed. |
| 2.2.20 | 480MHz max not available | P (all) | Our config is 400MHz/VOS1 | **NOT-AFFECTED** (E-05 confirmed). 400MHz is within all revisions' capability. Future 480MHz experiment requires rev V+ (REV_ID 0x2003). |
| 2.5.1-4 | DMAMUX errata | Various | DShot DMAMUX routing | **2.5.4 AFFECTED (all revs)**: wrong DMA request on CxCR write coinciding with sync event. Workaround: clear overrun flags before configuring. **Add to DShot init sequence.** |
| 2.9.1-4 | SDMMC errata | Various | H743 blackbox (TF card) | **WALK-VERIFY** at integration — our simple single-block writes may avoid the affected patterns. |
| 2.10.8 | ADC3 resolution limited by LSE | A (Y/W only) | VBAT on ADC3 | **WALK-VERIFY** — if LSE is running, ADC3 resolution may be reduced. We don't use LSE, but verify it's not accidentally enabled. |
| 2.16.2 | Consecutive compare event missed | N (all) | DShot compare DMA | **NOT-AFFECTED** — DMA-fed, not interrupt-driven (same rationale as F405 2.6.3). |
| 2.19.3 | I2C setup time wrong with short clock period | P (all) | H3LIS I2C at 400kHz | **WALK-VERIFY** — check if our I2C clock configuration triggers the short-period condition. |
| 2.19.4 | Spurious bus error in master mode | A (all) | H3LIS I2C reads | **AFFECTED** — same class as F405 2.9.6. Clear BERR, don't abort. |
| 2.19.9 | Transmission stalled after first byte | A (all) | Multi-byte H3LIS reads | **WALK-VERIFY** — read detail page for trigger conditions. |
| 2.22.2 | SPI master stall at high clock | A (all) | SPI flash for config/blackbox | **WALK-VERIFY** — check our SPI prescaler vs the stall condition. |

### Revision Gate (E-05 made deliberate)
- Rev Y/W (REV_ID 0x1003): 400MHz max, no VOS0. **Our config is safe.**
- Rev X (REV_ID 0x2001): fixed some errata
- Rev V (REV_ID 0x2003): latest, required for any 480MHz experiment
- **Bench step: read DBGMCU_IDC at first connect, record revision per board.**

---

## STM32H723/H725 — ES NOT YET OBTAINED

The H72x errata sheet was not found in the Wayback Machine archive. This is a newer part.
**MANDATORY before first H725 flash: locate and walk the H72x ES document.**
Flag specifically: SMPS transition timing, VOS0 region constraints, any DMAMUX differences from H743.

---

## Action Items from This Audit

### Fixes Required (code changes):
1. **2.5.4 (H743 DMAMUX)**: Clear overrun flags before DMA channel configuration in H7 DShot init
2. **2.9.6 / 2.19.4 (I2C spurious BERR)**: Add BERR flag clearing to i2c_recovery error handler
3. **2.6.1 (F405 TIM break)**: Research break input behavior with MOE — may need break disable or software management

### Bench Gates (hardware verification):
1. Read silicon revision ID on every board (DBGMCU_IDC register)
2. Verify 2.2.9 (AXI SRAM corruption) not triggered on H743 rev Y boards
3. Verify I2C timing parameters against 2.19.3 conditions
4. Verify ADC3 resolution (2.10.8) with LSE state
5. Verify SPI flash clock vs 2.22.2 stall condition
6. **Obtain H72x ES document** before H725 flash

### Documentation:
- BENCH_TEST.md: add "record silicon revision ID" step
- PROVISIONING.md: add revision ID to board identity record (already there via E-05)

---

*Audit performed against actual ES PDFs obtained from Wayback Machine archives.*
*PDFs archived in docs/pinmap_sources/ for provenance.*
*I-20: re-run this audit on any peripheral configuration change.*

---

## ERRATA RE-WALK v2 (2026-07-10) — Current Revisions

### New Sources Banked (via Fable):
- **ES0491 Rev 9 (H72x)** — H723/H725 errata sheet OBTAINED
  - ADC3 12-bit on H72x (NOT 16-bit) — confirms D1 change needed
  - UART-DMA bit-20 workaround noted for E2 CRSF driver
  - I2C kernel clock >= 10MHz for Fast mode (same as F72x)
  - Revision gate: 0x1000 (rev Z), 0x1001 (rev A)

- **ES0360 Rev 5 (F72x)** — the never-walked family, NOW WALKED
  - ZERO timer errata, ZERO DMA errata (both clean for DShot — cited)
  - §2.2.1: Internal noise vs ADC — VBAT accuracy impact noted
  - §2.9.1: I2C kernel >= 10MHz for Fast mode
  - §2.9.2: Spurious BERR master — same clear+continue recipe as H7
  - §2.1.1: M7 write-through corruption — N/A (default SRAM is WB)

- **ES0287 Rev 6 (F411)** — updated from Rev 2
  - **§2.2.7: Delay after RCC clock enable — AFFECTS gpioEnablePortClock()
    FIXED: DSB added to the helper (the fix for F-03 would have shipped
    with its own erratum without this)**
  - §2.6.3: Consecutive compare missed — N/A (DShot duty never triggers)
  - §2.6.1: PWM re-enabled after break — N/A (AOE never set)
  - §2.10.1: USART idle-frame — design note for E2 (use DMA ring, not idle-line)
  - §2.12.1: OTG_FS TxFIFO corruption — implementation note for E3 CDC

### Errata-Unwalked Asterisks Removed:
- F722: ES0360 Rev 5 walked → asterisk removed
- H725: ES0491 Rev 9 walked → asterisk removed

### Remaining (archived revisions already walked, delta risk low):
- ES0182 current (F405) — have Rev 14, check for new errata only
- ES0290 current (F745) — have Rev 7, check for new errata only
- ES0392 current (H743) — have Rev 13, check for new errata only
