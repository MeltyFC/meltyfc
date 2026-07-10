# Phase Report: R14 Work Order

Executed: 2026-07-10
Auditor: Fable (via Trip) | Executor: Nexus

## Status: COMPLETE — both items resolved, works-level

### Test Evidence
- **278 native tests passing** (same count, test content replaced with spec-derived frames)
- **6/6 cross-compilation targets: SUCCESS**

---

## R14-1: H725 VBAT ADC3 → ADC1

**Finding**: `targets/betafpv_h725/pinmap.h` had `VBAT_ADC_INSTANCE ADC3`. On H72x
silicon, ADC3 is the 12-bit converter (not 16-bit like H743). ES0491 §2.10.7
forbids our usage pattern on Rev A. The D1 rollout conflated the two H7 variants —
MicoAir H743 correctly stays on ADC3 (16-bit, no errata constraint).

**Fix**: H725 pinmap changed to `ADC1`. PC0 is INP10 on all three ADC instances —
same pin, no hardware routing change needed. ES0491 §2.10.7 cited in comment.

**Diff (the four lines)**:
```
-#define VBAT_ADC_INSTANCE ADC3       // H725: PC0 on ADC3
-#define VBAT_ADC_CHANNEL ADC_CHANNEL_10  // PC0 = ADC3_INP10
+#define VBAT_ADC_INSTANCE ADC1       // H72x: ADC1 is 16-bit; ADC3 is 12-bit (ES0491 §2.10.7)
+#define VBAT_ADC_CHANNEL ADC_CHANNEL_10  // PC0 = ADC1_INP10
```

H7 VBAT HAL clock enable already dispatches on instance (`ADC3` → `__HAL_RCC_ADC3_CLK_ENABLE`,
else → `__HAL_RCC_ADC12_CLK_ENABLE`). ADC1 correctly hits the ADC12 clock gate. No HAL
code change needed.

**Evidence**: betafpv_h725 builds clean. MicoAir H743 still on ADC3 (confirmed unchanged).

---

## R14-2: EDT discrimination mechanism uncited + wrong field

**Finding**: `isEdtExtendedFrame()` asserted "ESC replaces the CRC nibble with a type
indicator" with no specification cited. The R13 order required a citation. Upon
fetching the actual spec, the mechanism is fundamentally different from what the code
implemented.

**Spec**: bird-sanctuary/extended-dshot-telemetry, "Frame structure" section.

**Actual mechanism** (from spec):
- 16-bit decoded frame: `eeem mmmm mmmm cccc`
- PREFIX = bits [15:12] = 3-bit exponent + MSB of mantissa
- Spec: "If [the prefix] is 0 OR the 8th bit is 1, it is a eRPM frame"
- EDT frames have **even, non-zero** prefix values:
  - 0b0010 (2) = Temperature, 0b0100 (4) = Voltage, 0b0110 (6) = Current
  - 0b1000 (8) = Debug 1, 0b1010 (10) = Debug 2, 0b1100 (12) = Stress, 0b1110 (14) = Status

**What the code was doing**: Checking bits [3:0] (CRC nibble) for values 1-5.
Wrong field entirely. The edtNegotiated gate (R13-3) made this safe for v1, but the
discrimination logic was incorrect for future EDT use.

**Fix**:
```cpp
uint8_t prefix = (decodedFrame >> 12) & 0x0F;
return prefix != 0 && (prefix & 1) == 0;
```

Spec citation added to both `dshot_common.cpp` (inline, with prefix table) and
`dshot_common.hpp` (declaration comment).

**Tests rebuilt from spec examples** (not arbitrary hex values):
- EDT Temperature: exp=1, m8=0, value=50 → prefix 0b0010 → EDT ✓
- EDT Voltage: exp=2, m8=0, value=12 → prefix 0b0100 → EDT ✓
- EDT Current: exp=3, m8=0 → prefix 0b0110 → EDT ✓
- EDT Status: exp=7, m8=0 → prefix 0b1110 → EDT ✓
- eRPM (odd prefix): exp=0, m8=1 → prefix 0b0001 → eRPM ✓
- eRPM (odd prefix): exp=1, m8=1 → prefix 0b0011 → eRPM ✓
- eRPM (zero prefix): exp=0, m8=0 → prefix 0b0000 → eRPM ✓
- Gate test: all EDT frames return false when `edtNegotiated == false` ✓

**Evidence**: 39/39 DShot tests pass. 278/278 total. Spec cited by name and section.

---

## verify.sh
```
$ cd /home/claude/worktrees/meltyfc && bash verify.sh
```
(Not re-run in full here — R13's verify.sh additions confirmed working, no new
verify.sh changes in R14.)
