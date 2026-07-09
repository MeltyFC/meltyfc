# MeltyFC — CHIP EXPANSION PHASE 2: the rest of the popular FC market
### Spec for Opus/Nexus · 2026-07-09 · commit as docs/EXPANSION_PHASE2.md
### GATED: work starts only after Round-6 acceptance (DONE — verify.sh green, phase report posted).
### All v1 rulings, I-1..I-12, VERIFICATION.md, and PORTING.md patterns remain binding.

## 0. SCOPE

| Tier | Chips | Family Status | Why |
|------|-------|---------------|-----|
| A | F411, F401 | f4 variants — near-free | Budget-whoop install base; largest scavengeable FC population |
| B | H723/H725 | h7 variants — moderate | Current-gen performance boards (550MHz); PWR trap goes live |
| C | AT32F435 | NEW VENDOR — its own project | Budget-AIO giant; not STM32, new HAL family |
| OUT | G4, F3/F1, L4, RP2040/ESP32 | — | No boards, dead, or wrong market |

## TIER A — F411/F401

A1: Flash-gated feature tiers (MELTYFC_TIER_FULL/STD/MIN). Safety never tiered.
A2: Reference boards from BF unified config. F411=512K/128K, F401=256K/64K.
A3: Timer/DMA allocation — F411 has fewer than F405.

## TIER B — H723/H725

B1: H7_PWR_SUPPLY required per-board field, no default, #error if missing.
B2: Clock 550MHz needs VOS0 — conservative first (400-520MHz).
B3: Memory map deltas — D2 range per-chip, unified constants.
B4: Reference board from BF config.

## TIER C — AT32F435 (go/no-go)

GO criteria: F4+F7+H7 hardware-validated, named target board, demand signal.
New hal/at32/ family, PlatformIO platform handling, register-name translation.

## CROSS-CUTTING

1. Ldscript + map-gate on day one for every target.
2. I-12 boot assert in every target.
3. Disarm-frame prefill in HAL contract.
4. PORTING.md + TARGETS.md for community contributors.
5. Phase reports per tier.

## ORDER

Tier A (F411 first) -> Tier B (H72x) -> Tier C go/no-go -> AT32 if GO.
