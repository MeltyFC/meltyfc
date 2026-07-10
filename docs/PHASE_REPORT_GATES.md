# Phase Report: Gate Matrix Implementation

Executed: 2026-07-10
Directive: Trip (P-R1/P-R2 standing rules) | Executor: Nexus

## Status: G-2 through G-5, G-8 implemented and verified. G-1/G-6/G-7 deferred (see notes).

### Test Evidence
- **295 native tests passing** (unchanged — gates are verify.sh structural)
- **6/6 cross-compilation targets: SUCCESS**
- All implemented gates verified passing on clean tree

---

## Implemented Gates

### G-2: Peripheral-literal gate
Fails on bare `TIM[0-9]+|ADC[1-3]|USART|UART|SPI|I2C` literals in `src/hal/` and
`src/` outside pinmap headers, route-table consumption lines, and `// PERIPH-LITERAL-OK`
annotations. Allowlists HAL type names (TIM_HandleTypeDef, ADC_CHANNEL, etc.), RCC
clock enables, and DMAMUX requests.

### G-3: Safety-test manifest
9 required test function names checked by grep against `test/`. Missing = build fail.
List: `test_failsafe_recovery_requires_full_gesture`, `test_frame_age_just_over_failsafe_triggers`,
`test_choke_nan_armed`, `test_defaults_pass_validation`, `test_cannot_arm_lvc_critical`,
`test_clamp_config_legacy_failsafe_floor`, `test_validate_accel_saturation_3000rpm_60mm`,
`test_failsafe_with_lvc_critical_blocks_rearm`, `test_frame_age_triggers_with_valid_frames`.

### G-4: meltyGpioInit wrapper + gate
New wrapper in `src/hal/common/gpio_init.h`: `meltyGpioInit(port, init)` =
`gpioEnablePortClock(port) + HAL_GPIO_Init(port, init)`. All 9 bare `HAL_GPIO_Init`
calls in HAL drivers replaced. verify.sh fails on bare `HAL_GPIO_Init(` outside
the wrapper's own file. Unclocked-port writes are now impossible to reintroduce.

**Files converted**: dshot_hal_{f4,f7,h7}.cpp, vbat_hal_{f4,f7,h7}.cpp,
ws2812_hal_{f4,f7,h7}.cpp (9 files total).

### G-5: `|| true` allowlist self-policing
Every `|| true` in verify.sh must carry `# ALLOW-TRUE: reason`. All 14 existing
instances annotated. Unannotated `|| true` = build fail. G-5's own grep pattern
excluded by self-reference markers.

### G-8: DMA-mode explicitness gate
Two checks: (1) `DMA_CIRCULAR` in `src/hal/` → fail (I-3). (2) Files with
`HAL_DMA_Init` must have `.Init.Mode` assignment — missing = fail.

---

## Deferred Gates (require deeper integration work)

### G-1: `[[nodiscard]]` sweep
Requires auditing all 47+ status-returning functions across the codebase and adding
`[[nodiscard]]` to each declaration + `-Werror=unused-result` in build flags. This
is a standalone sweep best done as its own commit with the build verified under
`-Werror`. Deferred to avoid conflating with the structural gates.

### G-6: Unit-suffix gate
Regex matching `(timeout|delay|period|time|interval)[,)]` without `Ms|Us|Cycles|Hz`
suffix. Requires careful allowlist construction to avoid false positives on struct
member names and HAL type names. Deferred for tuning.

### G-7: Doc-sync count gates
Requires extracting PARAM_REGISTRY_SIZE from the native build and comparing to
PARAMS.md row count. Deferred until PARAMS.md is updated to current registry.
