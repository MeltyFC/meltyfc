// MeltyFC — Reset Cause Forensics (P-04 / L3)
// At boot, read RCC_CSR to determine WHY we reset.
// Pure logic — the register read is HAL-portable.
//
// Every professional product does this. It's the difference between
// "it rebooted at the event" and "IWDG fired, here's why."

#pragma once

#include <cstdint>

namespace melty {

enum class ResetCause : uint8_t {
    POWER_ON,       // Normal power-on
    WATCHDOG,       // IWDG or WWDG triggered — the interesting one
    SOFTWARE,       // NVIC_SystemReset() called
    PIN_RESET,      // NRST pin held low
    BROWNOUT,       // BOR detected (if BOR level configured)
    UNKNOWN,        // Multiple or unrecognized flags
};

struct ResetInfo {
    ResetCause cause;
    uint32_t rawFlags;  // Raw RCC_CSR value for blackbox/CLI
};

// Read and clear reset flags. Call ONCE at boot, before anything else.
// The raw CSR value is captured before clearing so it's available
// for blackbox header and CLI `status` display.
//
// Implementation note: HAL-portable across F4/F7/H7:
//   uint32_t csr = RCC->CSR;
//   RCC->CSR |= RCC_CSR_RMVF;  // Clear flags
//   Decode: IWDGRSTF, WWDGRSTF, SFTRSTF, PORRSTF, PINRSTF, BORRSTF
ResetInfo readResetCause();

// Format reset cause as a human-readable string for CLI
const char* resetCauseString(ResetCause cause);

} // namespace melty
