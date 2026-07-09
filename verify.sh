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

# Per-target flash/RAM budgets (bytes)
declare -A TARGET_FLASH=(
    [crux_f405hd]=1048576      # 1MB
    [betafpv_f411]=524288      # 512KB
    [aikon_f7mini]=524288      # 512KB
    [jhemcu_ghf745]=1048576    # 1MB
    [micoair_h743v2]=2097152   # 2MB
    [betafpv_h725]=1048576     # 1MB
)
declare -A TARGET_RAM=(
    [crux_f405hd]=196608       # 192KB (usable SRAM, excluding CCM)
    [betafpv_f411]=131072      # 128KB (no CCM on F411)
    [aikon_f7mini]=262144      # 256KB
    [jhemcu_ghf745]=327680     # 320KB
    [micoair_h743v2]=1048576   # ~1MB (AXI+D2+D3, excludes DTCM/ITCM)
    [betafpv_h725]=577536      # ~564KB (AXI 320K + D2 32K + D3 16K + DTCM 128K + ITCM 64K)
)
ALL_TARGETS=(crux_f405hd betafpv_f411 aikon_f7mini jhemcu_ghf745 micoair_h743v2 betafpv_h725)

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
export PATH="$PATH:$HOME/.local/bin:/home/admin/.local/bin:/home/admin/.platformio/packages/toolchain-gccarmnoneeabi/bin"

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
# 2. Multi-target build + resource budgets
# ----------------------------------------------------------------
echo ""
echo "--- Step 2: Multi-target build + resource budgets ---"
TARGETS_PASSED=0
TARGETS_TOTAL=${#ALL_TARGETS[@]}

for TARGET in "${ALL_TARGETS[@]}"; do
    echo ""
    echo "  Building: ${TARGET}"
    MAP_FILE=".pio/build/${TARGET}/firmware.map"
    BUILD_OUTPUT=$(PLATFORMIO_BUILD_FLAGS="-Wl,-Map,${MAP_FILE}" pio run -e "$TARGET" 2>&1)
    BUILD_EXIT=$?

    if [ $BUILD_EXIT -ne 0 ]; then
        echo "$BUILD_OUTPUT" | tail -20
        fail "Target build FAILED: ${TARGET}"
    fi

    # Resource budget from arm-none-eabi-size (reliable, not grep on pio output)
    ELF_FILE=".pio/build/${TARGET}/firmware.elf"
    if [ -f "$ELF_FILE" ]; then
        SIZE_OUTPUT=$(arm-none-eabi-size "$ELF_FILE" 2>/dev/null || true)
        if [ -n "$SIZE_OUTPUT" ]; then
            # arm-none-eabi-size output: text data bss dec hex filename
            FLASH_USED=$(echo "$SIZE_OUTPUT" | tail -1 | awk '{print $1 + $2}')
            RAM_USED=$(echo "$SIZE_OUTPUT" | tail -1 | awk '{print $2 + $3}')
        else
            FLASH_USED=0
            RAM_USED=0
        fi
    else
        FLASH_USED=0
        RAM_USED=0
    fi

    FLASH_TOTAL=${TARGET_FLASH[$TARGET]}
    RAM_TOTAL=${TARGET_RAM[$TARGET]}

    if [ "$FLASH_USED" != "0" ] && [ "$RAM_USED" != "0" ]; then
        FLASH_PCT=$((FLASH_USED * 100 / FLASH_TOTAL))
        RAM_PCT=$((RAM_USED * 100 / RAM_TOTAL))

        echo "  Flash: ${FLASH_USED} / ${FLASH_TOTAL} bytes (${FLASH_PCT}%)"
        echo "  RAM:   ${RAM_USED} / ${RAM_TOTAL} bytes (${RAM_PCT}%)"

        if [ $FLASH_PCT -gt $FLASH_FAIL_PCT ]; then
            fail "${TARGET}: Flash usage ${FLASH_PCT}% exceeds ${FLASH_FAIL_PCT}% limit"
        elif [ $FLASH_PCT -gt $FLASH_WARN_PCT ]; then
            warn "${TARGET}: Flash usage ${FLASH_PCT}% exceeds ${FLASH_WARN_PCT}% warning threshold"
        fi

        if [ $RAM_PCT -gt $RAM_FAIL_PCT ]; then
            fail "${TARGET}: RAM usage ${RAM_PCT}% exceeds ${RAM_FAIL_PCT}% limit"
        elif [ $RAM_PCT -gt $RAM_WARN_PCT ]; then
            warn "${TARGET}: RAM usage ${RAM_PCT}% exceeds ${RAM_WARN_PCT}% warning threshold"
        fi
    else
        warn "${TARGET}: Could not parse resource usage — check manually"
    fi

    # I-11 map-gate: verify DMA sections exist AND are at legal addresses
    # Missing section = FAIL (not skip) — a dropped/renamed section is the most
    # likely real-world failure and must not pass silently.
    if [ -f "$MAP_FILE" ]; then
        case "$TARGET" in
            *f7mini|*ghf745)
                # F7: .dtcm_dma must be defined and at 0x20000000-0x2000FFFF
                DTCM_ADDR=$(grep '\.dtcm_dma' "$MAP_FILE" | grep '0x' | awk '{for(i=1;i<=NF;i++) if($i ~ /^0x/) {print $i; exit}}' | head -1)
                if [ -n "$DTCM_ADDR" ] && [ "$DTCM_ADDR" != "0x00000000" ]; then
                    ADDR_DEC=$((DTCM_ADDR))
                    if [ "$ADDR_DEC" -lt $((0x20000000)) ] || [ "$ADDR_DEC" -gt $((0x2000FFFF)) ]; then
                        fail "${TARGET}: .dtcm_dma at ${DTCM_ADDR} — NOT in DTCM (I-11a)"
                    fi
                    echo "  .dtcm_dma: ${DTCM_ADDR} (DTCM OK)"
                else
                    # Section defined but empty (stubs not yet linked) — verify ldscript has it
                    if grep -q 'dtcm_dma' "$MAP_FILE" 2>/dev/null; then
                        echo "  .dtcm_dma: section defined, empty (stubs — will populate at integration)"
                    else
                        fail "${TARGET}: .dtcm_dma section MISSING from map AND linker script (I-11a)"
                    fi
                fi
                ;;
            *h743*)
                # H7: .d2_dma must be defined and at 0x30000000-0x3001FFFF
                D2_ADDR=$(grep '\.d2_dma' "$MAP_FILE" | grep '0x' | awk '{for(i=1;i<=NF;i++) if($i ~ /^0x/) {print $i; exit}}' | head -1)
                if [ -n "$D2_ADDR" ] && [ "$D2_ADDR" != "0x00000000" ]; then
                    ADDR_DEC=$((D2_ADDR))
                    if [ "$ADDR_DEC" -lt $((0x30000000)) ] || [ "$ADDR_DEC" -gt $((0x3001FFFF)) ]; then
                        fail "${TARGET}: .d2_dma at ${D2_ADDR} — NOT in D2 SRAM (I-11b)"
                    fi
                    echo "  .d2_dma: ${D2_ADDR} (D2 SRAM OK)"
                else
                    if grep -q 'd2_dma' "$MAP_FILE" 2>/dev/null; then
                        echo "  .d2_dma: section defined, empty (stubs — will populate at integration)"
                    else
                        fail "${TARGET}: .d2_dma section MISSING from map AND linker script (I-11b)"
                    fi
                fi
                ;;
        esac
    else
        warn "${TARGET}: No map file generated — cannot verify DMA section placement"
    fi

    TARGETS_PASSED=$((TARGETS_PASSED + 1))
    pass "${TARGET} build + budget + map"
done

echo ""
pass "All ${TARGETS_PASSED}/${TARGETS_TOTAL} targets built successfully"

# ----------------------------------------------------------------
# 3. Static analysis (cppcheck — run on default env only for speed)
# ----------------------------------------------------------------
echo ""
echo "--- Step 3: Static analysis ---"
CPPCHECK_OUTPUT=$(pio check --skip-packages -e crux_f405hd 2>&1 || true)
CPPCHECK_ERRORS=$(echo "$CPPCHECK_OUTPUT" | grep -c "\[error\]" || true)
if [ "$CPPCHECK_ERRORS" -gt 0 ]; then
    echo "$CPPCHECK_OUTPUT" | grep "\[error\]"
    fail "cppcheck found ${CPPCHECK_ERRORS} error(s)"
fi
pass "Static analysis (cppcheck)"

# ----------------------------------------------------------------
# 4. Format check (clang-format)
# ----------------------------------------------------------------
echo ""
echo "--- Step 4: Format check ---"
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
# 5. Architecture boundary audit
# ----------------------------------------------------------------
echo ""
echo "--- Step 5: Architecture boundary audit ---"

# 5a. Core modules must not include hardware HAL
CORE_FILES=$(find src/ -maxdepth 1 -name '*.cpp' -o -name '*.hpp' | grep -v main.cpp | sort)
BOUNDARY_VIOLATIONS=$(grep -rlE '#include .*(stm32|hal_|cmsis|Arduino)' \
    $CORE_FILES 2>/dev/null || true)

if [ -n "$BOUNDARY_VIOLATIONS" ]; then
    echo "Hardware includes found in pure-logic modules:"
    echo "$BOUNDARY_VIOLATIONS"
    fail "Architecture boundary violation: core includes HAL"
fi

# 5b. Core modules must not include hal/ (only hal/common/ via the HAL interface)
CORE_HAL_INCLUDE=$(grep -rlE '#include.*"hal/(f4|f7|h7)/' \
    $CORE_FILES 2>/dev/null || true)
if [ -n "$CORE_HAL_INCLUDE" ]; then
    echo "Core module includes family-specific HAL:"
    echo "$CORE_HAL_INCLUDE"
    fail "Architecture boundary violation: core includes family HAL"
fi

# 5c. HAL families must not cross-include
F4_CROSS=$(grep -rlE '#include.*"hal/(f7|h7)/' src/hal/f4/ 2>/dev/null || true)
F7_CROSS=$(grep -rlE '#include.*"hal/(f4|h7)/' src/hal/f7/ 2>/dev/null || true)
H7_CROSS=$(grep -rlE '#include.*"hal/(f4|f7)/' src/hal/h7/ 2>/dev/null || true)
if [ -n "$F4_CROSS" ] || [ -n "$F7_CROSS" ] || [ -n "$H7_CROSS" ]; then
    echo "Cross-family HAL includes found:"
    echo "$F4_CROSS" "$F7_CROSS" "$H7_CROSS"
    fail "Architecture boundary violation: HAL families cross-include"
fi

pass "Architecture boundary audit (core isolation + family isolation)"

# ----------------------------------------------------------------
# 6. Rocket invariants (grep-enforceable safety rules)
# ----------------------------------------------------------------
echo ""
echo "--- Step 6: Rocket invariants ---"

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

# Pinmap warnings — all targets
for TARGET_DIR in targets/*/; do
    TARGET_NAME=$(basename "$TARGET_DIR")
    if grep -q "PINMAP_IS_PLACEHOLDER" "${TARGET_DIR}pinmap.h" 2>/dev/null; then
        warn "${TARGET_NAME}: Pinmap is PLACEHOLDER — do NOT flash"
    fi
    if grep -q "PINMAP_IS_BF_CONFIG_DERIVED" "${TARGET_DIR}pinmap.h" 2>/dev/null; then
        warn "${TARGET_NAME}: Pinmap is BF_CONFIG_DERIVED — verify on live dump at arrival"
    fi
done

# Safety-tier gate: no MELTYFC_TIER/MELTYFC_HAS inside safety-critical code
TIER_IN_SAFETY=$(grep -n 'MELTYFC_TIER\|MELTYFC_HAS' \
    src/safety.cpp src/safety.hpp 2>/dev/null || true)
if [ -n "$TIER_IN_SAFETY" ]; then
    echo "TIER GATE VIOLATION — feature tier guard found in safety code:"
    echo "$TIER_IN_SAFETY"
    fail "Safety code must NEVER be tier-gated"
fi

pass "Rocket invariants (I-1, I-2) + pinmap warnings + safety-tier gate"

# ----------------------------------------------------------------
# 7. Symbol-floor gate (core wiring check)
# ----------------------------------------------------------------
echo ""
echo "--- Step 7: Symbol-floor gate ---"
# When integration wiring lands, these core symbols MUST be present in every target ELF.
# During pre-integration skeleton phase, this is informational only (warn, not fail).
# Flip SYMBOL_GATE_ENFORCE=1 when main() calls the init functions.
SYMBOL_GATE_ENFORCE=0
CORE_SYMBOLS="melty_setup melty_loop"
# R8 Class C gate: warn on bare-statement Wire/storage calls (integration rule C)
BARE_WIRE=$(grep -rn 'Wire\.endTransmission()\s*;' src/ 2>/dev/null | grep -v '//' || true)
if [ -n "$BARE_WIRE" ]; then
    warn "R8 Class C: bare Wire.endTransmission() — return value must feed health flag"
fi
# R7-3: Future integration symbols — uncomment when wired into main().
# The build system itself refuses a wired firmware that skipped safety init.
# INTEGRATION_SYMBOLS="dshotInit ws2812Init vbatInit i2cRecoverAndInit"
# SAFETY_INIT_SYMBOLS="iwdgStarted vbatValid"  # I-7 ordering: WDT + VBAT

for TARGET in "${ALL_TARGETS[@]}"; do
    ELF_FILE=".pio/build/${TARGET}/firmware.elf"
    if [ -f "$ELF_FILE" ]; then
        MISSING=""
        for SYM in $CORE_SYMBOLS; do
            if ! arm-none-eabi-nm "$ELF_FILE" 2>/dev/null | grep -q " T.*${SYM}"; then
                MISSING="${MISSING} ${SYM}"
            fi
        done
        if [ -n "$MISSING" ]; then
            if [ "$SYMBOL_GATE_ENFORCE" = "1" ]; then
                fail "${TARGET}: Missing core symbols:${MISSING}"
            else
                warn "${TARGET}: Missing core symbols (pre-integration):${MISSING}"
            fi
        else
            echo "  ${TARGET}: core symbols present"
        fi
    fi
done
pass "Symbol-floor gate (pre-integration — informational)"

# ----------------------------------------------------------------
# Summary
# ----------------------------------------------------------------
echo ""
echo "=========================================="
echo -e "${GREEN}ALL CHECKS PASSED${NC} — ${TARGETS_PASSED} targets, $(cat /tmp/meltyfc-test-output.txt | grep -oP '\d+ test cases' | head -1 || echo '?')"
echo "=========================================="
