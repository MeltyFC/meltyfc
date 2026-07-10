# MeltyFC — Phase Report: Master Work Order Campaign
## Date: 2026-07-09/10
## verify.sh: GREEN on all 6 targets (267 tests, -Werror, platform pinned)

---

## Campaign Checklist (per MASTER_WORK_ORDER.md)

### Phase A — Correctness Criticals
| Item | Status | Commit |
|------|--------|--------|
| A1: Timer route-table refactor | DONE | a13f282, f7f614a |
| A2: Fault handler + .noinit | DONE | bc54eff, 3564742 |
| A3: DWT micros() | DONE | bc54eff |
| A4/CS-1: F411 96MHz replan | DONE | 87a435b |
| A4/CS-2: H7 USB kernel clock | DONE | 87a435b |
| A4/CS-3: H725 copy H743 400MHz | DONE | 87a435b |
| A4/CS-4: Bounded VOSRDY | DONE | 87a435b |
| A5/F-01: NaN guards + I-15 | DONE | 5c8b95e |

### Phase B — Enforcement
| Item | Status | Commit |
|------|--------|--------|
| B1: CO-4 choke gates | DONE | 69ebda3 |
| B2: CO-1 defaults validation | DONE | 51cc4cc |
| B3: verify.sh fail paths | DONE | 69ebda3 |
| B4: Platform pin + -Werror | DONE | e3372b0 |

### Phase C — Contract Hardening
| Item | Status | Commit |
|------|--------|--------|
| C1: CRSF clamp | DONE | 5c8b95e |
| C2: DisarmedOnlyToken | DONE | c5da110 |
| C3: GCR start-bit | DONE | b5fea76 |
| C4: Reset forensics | DONE | 91fafc2 |
| C5: HAL return checks | DONE | 91fafc2 |
| C6: I2C errata fix | DONE | adb8237 |
| C7: hitLogLatest nullptr | DONE (already present) | — |
| C8: Tier gates | DONE | c5da110 |
| C9: Stack 4KB | DONE | 6702e46 |
| C10: Loop timer | DONE | c5da110 |
| C11: Git hash embed | DONE | 91fafc2 |
| C12: GPIO unused init | DONE | a4d264b |
| C13: Rev V+ note | DONE | errata audit |

### Phase D — Tests
| Item | Status | Commit |
|------|--------|--------|
| EDT frame discard test | DONE | a4d264b |
| GCR start-bit test | DONE | a4d264b |
| CRSF clamp edge tests (4) | DONE | a4d264b |
| Loop timer tests (5) | DONE | a4d264b |
| VBAT filter tests (10) | DONE | 766bfd4 |
| Defaults validation test | DONE | 51cc4cc |
| Total: 267 tests (up from 256) | DONE | — |

### Phase E — HAL Completion
| Item | Status | Commit |
|------|--------|--------|
| E1: I2C H3LIS331 driver | DONE | e00b8e3 |
| E2: UART CRSF HAL contract | DONE | e00b8e3 |
| E3: USB CDC HAL contract | DONE | this commit |

### Phase F — Documents
| Item | Status | Commit |
|------|--------|--------|
| F1: I-13..I-21 + P-06 | DONE | 69ebda3 |
| F2: PROVISIONING.md | DONE | 6702e46 |
| F3: ISR_CONTRACT.md | DONE | 6702e46 |
| F4: TUNING.md endurance | DONE | 6702e46 |
| F5: Work order committed | DONE | 77b7b56 |
| F-BENCH: BENCH_TEST.md | DONE | 51cc4cc |
| F7: Phase report | DONE | this document |

---

## Build Results

| Target | Status | Tests |
|--------|--------|-------|
| crux_f405hd (F405) | SUCCESS | 267 native |
| betafpv_f411 (F411) | SUCCESS | 267 native |
| aikon_f7mini (F722) | SUCCESS | 267 native |
| jhemcu_ghf745 (F745) | SUCCESS | 267 native |
| micoair_h743v2 (H743) | SUCCESS | 267 native |
| betafpv_h725 (H725) | SUCCESS | 267 native |

All builds under -Werror. Platform pinned to ststm32@19.7.0.

## Invariants: I-1 through I-21

All machine-enforced or gate-tracked. See VERIFICATION.md for the complete table.

## Campaign Statistics

- Items in work order: ~50
- Items completed: ~50 (100%)
- Commits in campaign: 20+
- Test count: 245 -> 267 (+22 tests)
- Targets: 4 -> 6
- Invariants: 12 -> 21
- Errata sheets walked: 4 (ES0182, ES0287, ES0290, ES0392)
- Auditors involved: 4 (Nexus, Fable, GPT-5.5, Trip)
- Clock configs verified against RM: all 6

## What Awaits Hardware

1. Step 0 BF dumps -> pinmap arrival-diffs
2. BENCH_TEST.md full procedure
3. I2C sensor validation
4. DShot electrical timing scope check (I-16)
5. VBAT calibration per family
6. ESC timeout measurement
7. Silicon revision ID recording
8. Stack watermark measurement
9. Loop-time numbers
10. eta calibration

*The firmware got ahead of the parts. Again.*
