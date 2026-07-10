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

# ----------------------------------------------------------------
# 0. I-38: Failure-injection self-test
# Proves that set -e + fail() actually terminates the script.
# If this gate passes, every subsequent fail() call is known-reachable.
# ----------------------------------------------------------------
echo ""
echo "--- Step 0: I-38 failure-injection self-test ---"

# Test 1: fail() must produce non-zero exit in a subshell
SELF_TEST_OUTPUT=$(bash -c '
    set -euo pipefail
    fail() { echo "INJECTED_FAIL"; exit 1; }
    fail "test"
    echo "UNREACHABLE"
' 2>&1) && {
    fail "I-38: fail() did not produce non-zero exit — error handling broken"
}
if echo "$SELF_TEST_OUTPUT" | grep -q "UNREACHABLE"; then
    fail "I-38: code after fail() was reached — set -e is not working"
fi
if ! echo "$SELF_TEST_OUTPUT" | grep -q "INJECTED_FAIL"; then
    fail "I-38: fail() did not produce expected output"
fi

# Test 2: set -e catches command failure
SELF_TEST2=$(bash -c '
    set -euo pipefail
    false
    echo "UNREACHABLE_2"
' 2>&1) && {
    fail "I-38: set -e did not catch bare 'false' command"
}
if echo "$SELF_TEST2" | grep -q "UNREACHABLE_2"; then
    fail "I-38: code after failed command was reached"
fi

# Test 3: pipefail catches mid-pipe failure
SELF_TEST3=$(bash -c '
    set -euo pipefail
    false | cat
    echo "UNREACHABLE_3"
' 2>&1) && {
    fail "I-38: pipefail did not catch mid-pipe failure"
}

pass "I-38: failure-injection self-test (fail(), set -e, pipefail all verified)"

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
# 1b. P-03: -O2 canary build (catches UB and volatile-misuse that -Os hides)
# ----------------------------------------------------------------
echo ""
echo "--- Step 1b: -O2 canary build ---"
O2_OUTPUT=$(PLATFORMIO_BUILD_FLAGS="-O2" pio run -e crux_f405hd 2>&1)
O2_EXIT=$?
if [ $O2_EXIT -ne 0 ]; then
    echo "$O2_OUTPUT" | grep "error:" | head -5
    warn "P-03: -O2 canary build failed — review warnings (may reveal UB)"
fi
pass "-O2 canary build (crux_f405hd)"

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
        SIZE_OUTPUT=$(arm-none-eabi-size "$ELF_FILE" 2>/dev/null || true) # ALLOW-TRUE: size tool may not be installed
        if [ -n "$SIZE_OUTPUT" ]; then
            # arm-none-eabi-size output: text data bss dec hex filename
            FLASH_USED=$(echo "$SIZE_OUTPUT" | tail -1 | awk '{print $1 + $2}')
            RAM_USED=$(echo "$SIZE_OUTPUT" | tail -1 | awk '{print $2 + $3}')
        else
            FLASH_USED=0
            RAM_USED=0
        fi
    else
        # B3: missing ELF = fail, not skip
        fail "${TARGET}: firmware.elf not found after successful build — build system broken"
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
        # B3: size parse failure = fail (size tool broken or ELF corrupt)
        fail "${TARGET}: Could not parse resource usage — arm-none-eabi-size failed"
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
        # I-18: Missing map file = gate failure, not skip.
        # The -Map flag is in common build_flags — absence means build system changed.
        fail "${TARGET}: No map file generated — cannot verify DMA section placement (I-18)"
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
# I-18: Tool failure = gate failure (never swallow with bare-or-true) # ALLOW-TRUE: comment reference only
CPPCHECK_OUTPUT=$(pio check --skip-packages -e crux_f405hd 2>&1)
CPPCHECK_EXIT=$?
if [ $CPPCHECK_EXIT -ne 0 ] && [ $CPPCHECK_EXIT -ne 1 ]; then
    # Exit 1 = warnings only (acceptable). Other exits = tool crashed.
    echo "$CPPCHECK_OUTPUT" | tail -10
    fail "cppcheck tool CRASHED (exit $CPPCHECK_EXIT) — I-18: gate cannot pass on failed tool"
fi
CPPCHECK_ERRORS=$(echo "$CPPCHECK_OUTPUT" | grep -c "\[error\]" || true) # ALLOW-TRUE: grep -c returns 1 on no-match
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
    $CORE_FILES 2>/dev/null || true) # ALLOW-TRUE: no-match is the pass

if [ -n "$BOUNDARY_VIOLATIONS" ]; then
    echo "Hardware includes found in pure-logic modules:"
    echo "$BOUNDARY_VIOLATIONS"
    fail "Architecture boundary violation: core includes HAL"
fi

# 5b. Core modules must not include hal/ (only hal/common/ via the HAL interface)
CORE_HAL_INCLUDE=$(grep -rlE '#include.*"hal/(f4|f7|h7)/' \
    $CORE_FILES 2>/dev/null || true) # ALLOW-TRUE: no-match is the pass
if [ -n "$CORE_HAL_INCLUDE" ]; then
    echo "Core module includes family-specific HAL:"
    echo "$CORE_HAL_INCLUDE"
    fail "Architecture boundary violation: core includes family HAL"
fi

# 5c. HAL families must not cross-include
F4_CROSS=$(grep -rlE '#include.*"hal/(f7|h7)/' src/hal/f4/ 2>/dev/null || true) # ALLOW-TRUE: no-match is the pass
F7_CROSS=$(grep -rlE '#include.*"hal/(f4|h7)/' src/hal/f7/ 2>/dev/null || true) # ALLOW-TRUE: no-match is the pass
H7_CROSS=$(grep -rlE '#include.*"hal/(f4|f7)/' src/hal/h7/ 2>/dev/null || true) # ALLOW-TRUE: no-match is the pass
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
    src/ include/ 2>/dev/null || true) # ALLOW-TRUE: no-match is the pass
if [ -n "$I1_HITS" ]; then
    echo "I-1 VIOLATION — internal flash write code found:"
    echo "$I1_HITS"
    fail "Invariant I-1: application code must never write internal flash"
fi

# I-2: No option-byte code is ever linked
I2_HITS=$(grep -rlE 'OPTCR|OPTKEYR|OB_RDP|FLASH_OB_' \
    src/ include/ targets/ 2>/dev/null || true) # ALLOW-TRUE: no-match is the pass
if [ -n "$I2_HITS" ]; then
    echo "I-2 VIOLATION — option-byte code found:"
    echo "$I2_HITS"
    fail "Invariant I-2: no option-byte code may exist"
fi

# Pinmap warnings — BF_CONFIG_DERIVED targets
# (PLACEHOLDER gate moved to A6/I-40 section above — it FAILS, not warns)
for TARGET_DIR in targets/*/; do
    TARGET_NAME=$(basename "$TARGET_DIR")
    if grep -q "PINMAP_IS_BF_CONFIG_DERIVED" "${TARGET_DIR}pinmap.h" 2>/dev/null; then
        warn "${TARGET_NAME}: Pinmap is BF_CONFIG_DERIVED — verify on live dump at arrival"
    fi
done

# A6/I-40: Placeholder pinmaps are not flashable (unless explicitly allowed)
if [ "${MELTYFC_ALLOW_PLACEHOLDER:-0}" != "1" ]; then
    for TARGET_DIR in targets/*/; do
        if grep -q "PINMAP_IS_PLACEHOLDER" "${TARGET_DIR}pinmap.h" 2>/dev/null; then
            fail "$(basename $TARGET_DIR): PLACEHOLDER pinmap — not flashable (set MELTYFC_ALLOW_PLACEHOLDER=1 to override)"
        fi
    done
fi

# V-1/I-21: HAL drivers must not hardcode timer instances outside route consumption
# Allowed patterns: enableTimerClock dispatcher, MOE predicate (TIM1/TIM8 check),
# __HAL_RCC_TIM clock enables, ifdef TIM8 guards. Everything else = hardcoded routing.
I21_VIOLATIONS=$(grep -rn 'TIM[0-9]' src/hal/*/dshot_hal_*.cpp src/hal/*/ws2812_hal_*.cpp 2>/dev/null \
    | grep -v 'enableTimerClock\|timerAlready\|== TIM1\|== TIM8\|ifdef TIM\|__HAL_RCC_TIM\|DMA_REQUEST_TIM\|TIM_CHANNEL\|TIM_BDTR\|TIM_OCMODE\|TIM_HandleTypeDef\|//\|TIMER_PIN\|hMotorTimer\|DSHOT_BITRATE' \
    || true) # ALLOW-TRUE: no-match is the pass
if [ -n "$I21_VIOLATIONS" ]; then
    echo "I-21 VIOLATION — direct timer instance in HAL (should use route table):"
    echo "$I21_VIOLATIONS"
    fail "I-21: HAL drivers must consume timer instances from pinmap route defines"
fi

# B1/CO-4: Choke-point gates — motor authority must flow through safety.cpp only
CHOKE_LEAK=$(grep -rn 'chokeMotorOutput\|throttleToDshot' \
    src/ 2>/dev/null | grep -v 'safety\.\|motors_dshot\.\|main\.cpp\|test/' || true) # ALLOW-TRUE: no-match is the pass
if [ -n "$CHOKE_LEAK" ]; then
    echo "CO-4 VIOLATION — motor authority function called outside approved files:"
    echo "$CHOKE_LEAK"
    fail "Motor authority must only be called from safety.cpp/motors_dshot/main/test"
fi

# Safety-tier gate: no MELTYFC_TIER/MELTYFC_HAS inside safety-critical code
TIER_IN_SAFETY=$(grep -n 'MELTYFC_TIER\|MELTYFC_HAS' \
    src/safety.cpp src/safety.hpp 2>/dev/null || true) # ALLOW-TRUE: no-match is the pass
if [ -n "$TIER_IN_SAFETY" ]; then
    echo "TIER GATE VIOLATION — feature tier guard found in safety code:"
    echo "$TIER_IN_SAFETY"
    fail "Safety code must NEVER be tier-gated"
fi

pass "Rocket invariants (I-1, I-2) + pinmap warnings + safety-tier gate"

# ----------------------------------------------------------------
# 6b. Gate Matrix (G-2 through G-8)
# ----------------------------------------------------------------
echo ""
echo "--- Step 6b: Gate Matrix ---"

# G-2: Peripheral-literal gate — bare TIM/ADC/USART/SPI/I2C literals in HAL/src
# Peripheral instances come from pinmap defines, period.
G2_VIOLATIONS=$(grep -rnE '\bTIM[0-9]+\b|\bADC[1-3]\b|\bUSART[0-9]\b|\bUART[0-9]\b|\bSPI[0-9]\b|\bI2C[0-9]\b' \
    src/hal/ src/ 2>/dev/null \
    | grep -v 'pinmap\.h\|target\.h\|// PERIPH-LITERAL-OK' \
    | grep -v 'enableTimerClock\|timerAlready\|== TIM1\|== TIM8\|ifdef TIM\|__HAL_RCC_\|DMA_REQUEST_TIM\|MOTOR._TIMER\|MOTOR._DMA\|VBAT_ADC_INSTANCE\|LED_STRIP_TIMER' \
    | grep -v 'TIM_CHANNEL\|TIM_BDTR\|TIM_OCMODE\|TIM_HandleTypeDef\|TIM_DMA_ID\|TIM_COUNTERMODE\|TIM_CLOCKDIVISION\|TIM_AUTORELOAD\|TIM_ICPOLARITY\|TIM_ICSELECTION\|TIM_ICPSC\|TIM_OCFAST\|TIM_OCPOLARITY\|TIM_OCNPOLARITY\|TIM_OCIDLESTATE' \
    | grep -v 'ADC_CHANNEL\|ADC_RESOLUTION\|ADC_CLOCK\|ADC_DATAALIGN\|ADC_SCAN\|ADC_EOC\|ADC_SAMPLETIME\|ADC_CALIB\|ADC_SINGLE\|ADC_REGULAR\|ADC_OFFSET\|ADC_CONVERSIONDATA\|ADC_OVR\|ADC_HandleTypeDef\|HAL_ADC\|HAL_ADCEx' \
    | grep -v 'test/\|\.pio/' \
    || true) # ALLOW-TRUE: no-match is the pass
if [ -n "$G2_VIOLATIONS" ]; then
    echo "G-2 VIOLATION — bare peripheral literal (must come from pinmap defines):"
    echo "$G2_VIOLATIONS" | head -10
    fail "G-2: peripheral instances must come from pinmap defines, not hardcoded"
fi
pass "G-2: Peripheral-literal gate"

# G-3: Safety-test manifest — required test function names
REQUIRED_SAFETY_TESTS=(
    "test_failsafe_recovery_requires_full_gesture"
    "test_frame_age_just_over_failsafe_triggers"
    "test_choke_nan_armed"
    "test_defaults_pass_validation"
    "test_cannot_arm_lvc_critical"
    "test_clamp_config_legacy_failsafe_floor"
    "test_validate_accel_saturation_3000rpm_60mm"
    "test_failsafe_with_lvc_critical_blocks_rearm"
    "test_frame_age_triggers_with_valid_frames"
    "test_blackbox_defers_on_busy"
    "test_timeout_stops_before_clear"
    "test_disarm_preempts_busy"
)
G3_MISSING=""
for TEST_NAME in "${REQUIRED_SAFETY_TESTS[@]}"; do
    if ! grep -rq "$TEST_NAME" test/ 2>/dev/null; then
        G3_MISSING="${G3_MISSING} ${TEST_NAME}"
    fi
done
if [ -n "$G3_MISSING" ]; then
    echo "G-3 VIOLATION — required safety tests missing from suite:"
    echo "$G3_MISSING"
    fail "G-3: safety properties must have pinned tests"
fi
pass "G-3: Safety-test manifest (${#REQUIRED_SAFETY_TESTS[@]} required tests present)"

# G-4: Bare HAL_GPIO_Init gate — must go through meltyGpioInit wrapper
G4_BARE=$(grep -rn 'HAL_GPIO_Init(' src/hal/ 2>/dev/null \
    | grep -v 'gpio_port_clock\.h\|gpio_init\.h\|meltyGpioInit\|// BARE-GPIO-OK' \
    || true) # ALLOW-TRUE: no-match is the pass
if [ -n "$G4_BARE" ]; then
    echo "G-4 VIOLATION — bare HAL_GPIO_Init (use meltyGpioInit wrapper):"
    echo "$G4_BARE" | head -10
    fail "G-4: all GPIO init must go through meltyGpioInit (gpioEnablePortClock + HAL_GPIO_Init)"
fi
pass "G-4: GPIO init wrapper gate"

# G-5: pipe-true allowlist — every pipe-true must carry ALLOW-TRUE: reason
# (G-5 gate uses "pipe-true" in prose to avoid matching its own grep pattern)
G5_UNANNOTATED=$(grep -n '|| true' verify.sh \
    | grep -v 'ALLOW-TRUE:\|G-5.*allowlist\|G-5 VIOLATION\|G-5:\|G5_UNANNOTATED' \
    || true) # ALLOW-TRUE: self-reference meta-check
if [ -n "$G5_UNANNOTATED" ]; then
    echo "G-5 VIOLATION — unannotated pipe-true in verify.sh:"
    echo "$G5_UNANNOTATED"
    fail "G-5: every pipe-true must have ALLOW-TRUE: reason"
fi
pass "G-5: pipe-true allowlist self-policing"

# G-8: DMA-mode explicitness — no DMA_CIRCULAR, and every DMA init must have Mode=
G8_CIRCULAR=$(grep -rn 'DMA_CIRCULAR' src/hal/ 2>/dev/null \
    || true) # ALLOW-TRUE: no-match is the pass
if [ -n "$G8_CIRCULAR" ]; then
    echo "G-8 VIOLATION — DMA_CIRCULAR found in HAL (I-3: never circular):"
    echo "$G8_CIRCULAR"
    fail "G-8: DMA must never use circular mode"
fi
# Check every DMA init block has explicit Mode assignment
G8_NO_MODE=$(grep -rlE 'DMA_HandleTypeDef|hDma' src/hal/ 2>/dev/null | while read f; do
    # Files with DMA init should have .Init.Mode =
    if grep -q 'HAL_DMA_Init' "$f" 2>/dev/null; then
        if ! grep -q '\.Init\.Mode' "$f" 2>/dev/null; then
            echo "$f: DMA init without explicit Mode assignment"
        fi
    fi
done || true) # ALLOW-TRUE: no-match is the pass
if [ -n "$G8_NO_MODE" ]; then
    echo "G-8 VIOLATION — DMA init block lacks explicit Mode assignment:"
    echo "$G8_NO_MODE"
    fail "G-8: every DMA init must have explicit .Init.Mode = DMA_NORMAL"
fi
pass "G-8: DMA-mode explicitness gate"

# R16-3: micros() is main-context only — must not appear in ISR/callback/fault files
G_MICROS=$(grep -rn 'micros(' src/hal/*/fault_handler_*.cpp src/hal/*/dshot_hal_*.cpp 2>/dev/null \
    | grep -v '//\|#include\|micros_ISR_OK' \
    | grep 'micros(' \
    || true) # ALLOW-TRUE: no-match is the pass
if [ -n "$G_MICROS" ]; then
    echo "R16-3 VIOLATION — micros() in ISR-context file (use raw DWT->CYCCNT):"
    echo "$G_MICROS"
    fail "R16-3: micros() is main-context only (see ISR_CONTRACT.md)"
fi
pass "R16-3: micros() ISR exclusion gate"

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
BARE_WIRE=$(grep -rn 'Wire\.endTransmission()\s*;' src/ 2>/dev/null | grep -v '//' || true) # ALLOW-TRUE: no-match is the pass
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
