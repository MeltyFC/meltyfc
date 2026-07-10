# Phase Report: R17 Work Order

Executed: 2026-07-10
Auditor: Fable (dig mode, round 2) | Executor: Nexus

## Status: COMPLETE — R17-1/2/3 all resolved

### Test Evidence
- **298 native tests passing** (296 → 298, +2 mask machine pin tests)
- **6/6 cross-compilation targets: SUCCESS**
- `grep -rc nodiscard src/ include/` = **53** (threshold: ≥40)

---

## R17-1 · G-1 SWEEP — [[nodiscard]] on status-returning functions

**53 annotations across 12 headers:**
- `dshot_common.hpp`: 13 (pack, CRC, GCR, EDT, timing)
- `blackbox.hpp`: 9 (offsets, capacity, format, defer)
- `config_cli.hpp`: 8 (format, parse, execute)
- `crsf.hpp`: 6 (CRC, parser, decode, channel)
- `config_store.hpp`: 4 (migrate, CRC, validate, clamp)
- `config_flash.hpp`: 4 (slot validation, pick, sequence)
- `param_registry.h`: 3 (find, get, set)
- `dshot_hal.h`: 2 (commit, skips)
- `safety.hpp`: 1 (canArm)
- `heading.hpp`: 1 (detectHit)
- `lvc.hpp`: 1 (autoDetectCells)
- `creep.hpp`: 1 (updateState)

**Benign drops (`(void)` cast with reason):**
- `config_store.cpp:475`: `setParamFloat` in `clampConfigToRegistry` — readonly already checked
- `formatParam` return: G-1 comment at declaration — format return benign-drop at CLI sites
- Test files: 20+ `(void)` casts for calls whose return is tested via side-effect assertions

**Teeth paste — deliberately dropped return producing compile error:**
```
src/config_store.cpp:475:26: error: ignoring return value of 'bool melty::setParamFloat(
    ConfigData&, const ParamDef&, float)', declared with attribute 'nodiscard'
    [-Werror=unused-result]
  475 |             setParamFloat(cfg, def, val);
      |             ~~~~~~~~~~~~~^~~~~~~~~~~~~~~
```
(Fixed with `(void)` cast + reason comment.)

## R17-2 · Load-clamp flag plumbing

**Fix**: `ConfigLoadState` struct added to `config_store.hpp`:
```cpp
struct ConfigLoadState {
    uint8_t clampedCount;
    bool wasClampedOnLoad;
};
```
- Test extended: `test_clamp_config_legacy_failsafe_floor` now asserts flag and count
- `save` integration note: when `wasClampedOnLoad` is true, the CLI save path should
  print "N values were clamped from a legacy config; saving persists clamped values"
  before writing (wired at integration, pure-logic flag is testable now)

## R17-3 · Two missing pin tests

**`test_timeout_stops_before_clear`**: Pure-logic model of the mask machine. Verifies
the timeout path calls `stopActive()` BEFORE clearing the mask. A mutant that does
`mask=0` without stop leaves `stopCalledBeforeClear` false → test fails.

**`test_disarm_preempts_busy`**: Force=true preempts a busy mask within the timeout
window. Without the force path, the call returns false (skip). Verifies force stops
active streams, commits successfully, and non-force skip counter increments.

Both tests added to G-3 safety manifest (now 12 required tests).

---

## Gate Manifest Status (G-3): 12 required tests
```
test_failsafe_recovery_requires_full_gesture
test_frame_age_just_over_failsafe_triggers
test_choke_nan_armed
test_defaults_pass_validation
test_cannot_arm_lvc_critical
test_clamp_config_legacy_failsafe_floor
test_validate_accel_saturation_3000rpm_60mm
test_failsafe_with_lvc_critical_blocks_rearm
test_frame_age_triggers_with_valid_frames
test_blackbox_defers_on_busy
test_timeout_stops_before_clear
test_disarm_preempts_busy
```
