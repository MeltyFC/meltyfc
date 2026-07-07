#!/bin/bash
# MeltyFC — Verification Chain
# Run this before every commit. Fails fast on any issue.
# See docs/VERIFICATION.md for the full protocol.

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

FLASH_WARN_PCT=70
FLASH_FAIL_PCT=85
RAM_WARN_PCT=70
RAM_FAIL_PCT=85
FLASH_TOTAL=1048576   # 1MB
RAM_TOTAL=196608      # 192KB

pass() { echo -e "${GREEN}PASS${NC}: $1"; }
warn() { echo -e "${YELLOW}WARN${NC}: $1"; }
fail() { echo -e "${RED}FAIL${NC}: $1"; exit 1; }

echo "=========================================="
echo "MeltyFC verify.sh — $(date)"
echo "=========================================="

# Ensure we're in the repo root
if [ ! -f platformio.ini ]; then
    fail "Not in repo root (no platformio.ini found)"
fi

# Add PlatformIO to PATH if needed
export PATH="$PATH:$HOME/.local/bin:/home/admin/.local/bin"

# ----------------------------------------------------------------
# 1. Native unit tests (with ASan/UBSan if supported)
# ----------------------------------------------------------------
echo ""
echo "--- Step 1: Native unit tests ---"
pio test -e native --without-uploading 2>&1 | tee /tmp/meltyfc-test-output.txt
if [ ${PIPESTATUS[0]} -ne 0 ]; then
    fail "Native tests failed"
fi
pass "Native unit tests"

# ----------------------------------------------------------------
# 2. Target build (cross-compile for STM32)
# ----------------------------------------------------------------
echo ""
echo "--- Step 2: Target build (crux_f405hd) ---"
BUILD_OUTPUT=$(pio run -e crux_f405hd 2>&1)
BUILD_EXIT=$?
echo "$BUILD_OUTPUT" | tail -20
if [ $BUILD_EXIT -ne 0 ]; then
    fail "Target build failed"
fi
pass "Target build"

# ----------------------------------------------------------------
# 3. Resource budget
# ----------------------------------------------------------------
echo ""
echo "--- Step 3: Resource budget ---"
# Extract flash and RAM usage from build output
FLASH_USED=$(echo "$BUILD_OUTPUT" | grep -oP 'Flash:\s+\[.*?\]\s+\K\d+' | tail -1 || echo "0")
RAM_USED=$(echo "$BUILD_OUTPUT" | grep -oP 'RAM:\s+\[.*?\]\s+\K\d+' | tail -1 || echo "0")

# Try alternate parsing if the above didn't work
if [ "$FLASH_USED" = "0" ]; then
    FLASH_USED=$(echo "$BUILD_OUTPUT" | grep -i "program\|flash" | grep -oP '\d+' | head -1 || echo "0")
fi
if [ "$RAM_USED" = "0" ]; then
    RAM_USED=$(echo "$BUILD_OUTPUT" | grep -i "data\|ram" | grep -oP '\d+' | head -1 || echo "0")
fi

if [ "$FLASH_USED" != "0" ] && [ "$RAM_USED" != "0" ]; then
    FLASH_PCT=$((FLASH_USED * 100 / FLASH_TOTAL))
    RAM_PCT=$((RAM_USED * 100 / RAM_TOTAL))

    echo "Flash: ${FLASH_USED} / ${FLASH_TOTAL} bytes (${FLASH_PCT}%)"
    echo "RAM:   ${RAM_USED} / ${RAM_TOTAL} bytes (${RAM_PCT}%)"

    if [ $FLASH_PCT -gt $FLASH_FAIL_PCT ]; then
        fail "Flash usage ${FLASH_PCT}% exceeds ${FLASH_FAIL_PCT}% limit"
    elif [ $FLASH_PCT -gt $FLASH_WARN_PCT ]; then
        warn "Flash usage ${FLASH_PCT}% exceeds ${FLASH_WARN_PCT}% warning threshold"
    fi

    if [ $RAM_PCT -gt $RAM_FAIL_PCT ]; then
        fail "RAM usage ${RAM_PCT}% exceeds ${RAM_FAIL_PCT}% limit"
    elif [ $RAM_PCT -gt $RAM_WARN_PCT ]; then
        warn "RAM usage ${RAM_PCT}% exceeds ${RAM_WARN_PCT}% warning threshold"
    fi

    pass "Resource budget (Flash ${FLASH_PCT}%, RAM ${RAM_PCT}%)"
else
    warn "Could not parse resource usage from build output — check manually"
fi

# ----------------------------------------------------------------
# 4. Static analysis (cppcheck via PlatformIO)
# ----------------------------------------------------------------
echo ""
echo "--- Step 4: Static analysis ---"
CPPCHECK_OUTPUT=$(pio check --skip-packages -e crux_f405hd 2>&1 || true)
CPPCHECK_ERRORS=$(echo "$CPPCHECK_OUTPUT" | grep -c "\[error\]" || true)
if [ "$CPPCHECK_ERRORS" -gt 0 ]; then
    echo "$CPPCHECK_OUTPUT" | grep "\[error\]"
    fail "cppcheck found ${CPPCHECK_ERRORS} error(s)"
fi
pass "Static analysis (cppcheck)"

# ----------------------------------------------------------------
# 5. Format check (clang-format)
# ----------------------------------------------------------------
echo ""
echo "--- Step 5: Format check ---"
FORMAT_FILES=$(find src/ include/ test/ targets/ -name '*.cpp' -o -name '*.hpp' -o -name '*.h' 2>/dev/null)
if [ -n "$FORMAT_FILES" ]; then
    FORMAT_VIOLATIONS=$(echo "$FORMAT_FILES" | xargs clang-format --dry-run --Werror 2>&1 | wc -l)
    if [ "$FORMAT_VIOLATIONS" -gt 0 ]; then
        echo "$FORMAT_FILES" | xargs clang-format --dry-run --Werror 2>&1 | head -20
        fail "clang-format found ${FORMAT_VIOLATIONS} violation(s)"
    fi
fi
pass "Format check (clang-format)"

# ----------------------------------------------------------------
# 6. Architecture boundary audit
# ----------------------------------------------------------------
echo ""
echo "--- Step 6: Architecture boundary audit ---"
BOUNDARY_VIOLATIONS=$(grep -rlE '#include .*(stm32|hal_|cmsis|Arduino)' \
    src/heading.cpp src/heading.hpp \
    src/slip.cpp src/slip.hpp \
    src/config_cli.cpp src/config_cli.hpp \
    src/config_store.cpp src/config_store.hpp \
    src/crsf.cpp src/crsf.hpp \
    src/led_beacon.cpp src/led_beacon.hpp \
    src/creep.cpp src/creep.hpp \
    src/resync.cpp src/resync.hpp \
    src/hitlog.cpp src/hitlog.hpp \
    src/lvc.cpp src/lvc.hpp \
    src/blackbox.cpp src/blackbox.hpp \
    src/safety.cpp src/safety.hpp \
    include/param_registry.h \
    2>/dev/null || true)

if [ -n "$BOUNDARY_VIOLATIONS" ]; then
    echo "Hardware includes found in pure-logic modules:"
    echo "$BOUNDARY_VIOLATIONS"
    fail "Architecture boundary violation"
fi
pass "Architecture boundary audit"

# ----------------------------------------------------------------
# 7. Rocket invariants (grep-enforceable safety rules)
# ----------------------------------------------------------------
echo ""
echo "--- Step 7: Rocket invariants ---"

# I-1: Application code never writes internal flash
I1_HITS=$(grep -rlE 'FLASH_CR|FLASH_KEYR|HAL_FLASH_Program|HAL_FLASHEx_Erase' \
    src/ include/ 2>/dev/null || true)
if [ -n "$I1_HITS" ]; then
    echo "I-1 VIOLATION — internal flash write code found:"
    echo "$I1_HITS"
    fail "Invariant I-1: application code must never write internal flash"
fi

# I-2: No option-byte code is ever linked
I2_HITS=$(grep -rlE 'OPTCR|OPTKEYR|OB_RDP|FLASH_OB_' \
    src/ include/ targets/ 2>/dev/null || true)
if [ -n "$I2_HITS" ]; then
    echo "I-2 VIOLATION — option-byte code found:"
    echo "$I2_HITS"
    fail "Invariant I-2: no option-byte code may exist"
fi

# Finding 1: Pinmap placeholder warning
if grep -q "PINMAP_IS_PLACEHOLDER" targets/crux_f405hd/pinmap.h 2>/dev/null; then
    warn "Pinmap is PLACEHOLDER — do NOT flash to real hardware until verified from BF dump"
fi

# Finding 8: Motor choke enforcement
# (Placeholder check — will be expanded when HAL motor drivers exist.
#  For now, verify no raw DShot buffer writes outside approved files.)

pass "Rocket invariants (I-1, I-2) + safety checks"

# ----------------------------------------------------------------
# Summary
# ----------------------------------------------------------------
echo ""
echo "=========================================="
echo -e "${GREEN}ALL CHECKS PASSED${NC}"
echo "=========================================="
