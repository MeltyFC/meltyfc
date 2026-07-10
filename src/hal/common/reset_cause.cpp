// MeltyFC — Reset Cause Implementation (V-3 / C4 / P-04)
// Reads RCC_CSR at boot, decodes the cause, clears the flags.
// Portable across F4/F7/H7 (RCC_CSR bit layout is identical on all).

#ifndef NATIVE_BUILD

#include "reset_cause.hpp"

#if defined(STM32F4xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32F7xx)
#include "stm32f7xx_hal.h"
#elif defined(STM32H7xx)
#include "stm32h7xx_hal.h"
#endif

namespace melty {

ResetInfo readResetCause() {
    ResetInfo info = {};
    // H7 uses RCC->RSR (Reset Status Register), F4/F7 use RCC->CSR
#if defined(STM32H7xx)
    info.rawFlags = RCC->RSR;
    if (info.rawFlags & RCC_RSR_IWDG1RSTF)      info.cause = ResetCause::WATCHDOG;
    else if (info.rawFlags & RCC_RSR_WWDG1RSTF)  info.cause = ResetCause::WATCHDOG;
    else if (info.rawFlags & RCC_RSR_SFTRSTF)     info.cause = ResetCause::SOFTWARE;
    else if (info.rawFlags & RCC_RSR_BORRSTF)     info.cause = ResetCause::BROWNOUT;
    else if (info.rawFlags & RCC_RSR_PINRSTF)     info.cause = ResetCause::PIN_RESET;
    else if (info.rawFlags & RCC_RSR_PORRSTF)     info.cause = ResetCause::POWER_ON;
    else info.cause = ResetCause::UNKNOWN;
    RCC->RSR |= RCC_RSR_RMVF;
#else
    info.rawFlags = RCC->CSR;
    if (info.rawFlags & RCC_CSR_IWDGRSTF)         info.cause = ResetCause::WATCHDOG;
    else if (info.rawFlags & RCC_CSR_WWDGRSTF)    info.cause = ResetCause::WATCHDOG;
    else if (info.rawFlags & RCC_CSR_SFTRSTF)     info.cause = ResetCause::SOFTWARE;
    else if (info.rawFlags & RCC_CSR_BORRSTF)     info.cause = ResetCause::BROWNOUT;
    else if (info.rawFlags & RCC_CSR_PINRSTF)     info.cause = ResetCause::PIN_RESET;
    else if (info.rawFlags & RCC_CSR_PORRSTF)     info.cause = ResetCause::POWER_ON;
    else info.cause = ResetCause::UNKNOWN;
    RCC->CSR |= RCC_CSR_RMVF;
#endif

    return info;
}

const char* resetCauseString(ResetCause cause) {
    switch (cause) {
        case ResetCause::POWER_ON:  return "POWER_ON";
        case ResetCause::WATCHDOG:  return "WATCHDOG";
        case ResetCause::SOFTWARE:  return "SOFTWARE";
        case ResetCause::PIN_RESET: return "PIN_RESET";
        case ResetCause::BROWNOUT:  return "BROWNOUT";
        case ResetCause::UNKNOWN:   return "UNKNOWN";
    }
    return "UNKNOWN"; // Should never reach — but no default: to keep -Wswitch alive (R8 rule)
}

} // namespace melty

#endif // NATIVE_BUILD
