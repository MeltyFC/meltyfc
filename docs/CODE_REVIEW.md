# MeltyFC Code Review — Round 1 (2026-07-06)
# See MeltyFC_Code_Review_2026-07-06.md for full content
# Committed from Fable's independent review

## Priority Fixes Required

### A. WILL CRASH ON BOARD
- A1: Misaligned float access in config_store.cpp (packed struct + reinterpret_cast)

### B. DEAD ON ARRIVAL
- B1: Bidir DShot CRC must be INVERTED (both directions)
- B2: eRPM period is mantissa*2^exponent, not literal 12-bit us

### C. CONTROL DIRECTION LANDMINES
- C1: Motor position vs thrust — probable 90° translation error
- C2: Inversion — phase direction never flips
- C3: Re-sync circular mean breaks at ±180°

### D. INTEGRATION REQUIREMENTS
- D1: DShot 1-47 command range guard
- D2: First-iteration dt guard
- D3: LVC not wired into safety machine
- D4: Creep arm gating
- D5: RPM hold semantics clarification
- D6: Config save-while-armed blocking
