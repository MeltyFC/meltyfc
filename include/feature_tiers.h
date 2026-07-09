// MeltyFC — Flash-Gated Feature Tiers
// Introduced by Phase 2 Tier A (F411/F401 support).
//
// Tiers:
//   MELTYFC_TIER_FULL (>=1MB): everything
//   MELTYFC_TIER_STD  (512KB): full control/safety; POV fonts capped; blackbox reduced
//   MELTYFC_TIER_MIN  (256KB): full control + ALL safety + beacon/state LEDs;
//                              POV display and blackbox compiled out
//
// RULE: Safety code is NEVER tier-gated. No tier #if may wrap anything in
// safety.cpp, the choke point, or failsafe paths. verify.sh grep-enforces this.
//
// Set per target in target.h via #define MELTYFC_TIER <tier>
// If not defined, defaults to FULL (backward compatible with existing targets).

#pragma once

// Tier levels (numeric for comparison)
#define MELTYFC_TIER_MIN  1   // 256KB flash (F401CC)
#define MELTYFC_TIER_STD  2   // 512KB flash (F411, F722)
#define MELTYFC_TIER_FULL 3   // >=1MB flash (F405, F745, H743)

// Default to FULL if not specified by target
#ifndef MELTYFC_TIER
#define MELTYFC_TIER MELTYFC_TIER_FULL
#endif

// Feature gates — use these in code, never raw tier comparisons
#define MELTYFC_HAS_POV_DISPLAY     (MELTYFC_TIER >= MELTYFC_TIER_STD)
#define MELTYFC_HAS_BLACKBOX        (MELTYFC_TIER >= MELTYFC_TIER_STD)
#define MELTYFC_HAS_FULL_POV_FONTS  (MELTYFC_TIER >= MELTYFC_TIER_FULL)
#define MELTYFC_HAS_FULL_BLACKBOX   (MELTYFC_TIER >= MELTYFC_TIER_FULL)
#define MELTYFC_HAS_CLI_EXTENDED    (MELTYFC_TIER >= MELTYFC_TIER_STD)

// Blackbox ring size by tier
#if MELTYFC_TIER >= MELTYFC_TIER_FULL
    #define BLACKBOX_RING_SECTORS 64
#elif MELTYFC_TIER >= MELTYFC_TIER_STD
    #define BLACKBOX_RING_SECTORS 16
#else
    #define BLACKBOX_RING_SECTORS 0   // compiled out
#endif

// POV font table size by tier
#if MELTYFC_TIER >= MELTYFC_TIER_FULL
    #define POV_MAX_GLYPHS 96    // full ASCII printable
#elif MELTYFC_TIER >= MELTYFC_TIER_STD
    #define POV_MAX_GLYPHS 36    // digits + caps only
#else
    #define POV_MAX_GLYPHS 0     // compiled out
#endif

// Tier name for CLI `list json` reporting
#if MELTYFC_TIER == MELTYFC_TIER_MIN
    #define MELTYFC_TIER_NAME "MIN"
#elif MELTYFC_TIER == MELTYFC_TIER_STD
    #define MELTYFC_TIER_NAME "STD"
#else
    #define MELTYFC_TIER_NAME "FULL"
#endif
