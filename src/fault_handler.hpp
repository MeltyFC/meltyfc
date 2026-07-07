// MeltyFC — Fault Handler (R3-2)
// HardFault/BusFault/UsageFault → force-kill motor timer+DMA,
// write breadcrumbs to .noinit RAM, spin for IWDG.
// Boot: if breadcrumbs present → ERROR blink + CLI fault dump.
//
// The fault STRUCT and CLI formatter are PURE LOGIC (testable natively).
// The actual handler and .noinit placement are target-specific.

#pragma once

#include <cstdint>

namespace melty {

// Magic number to validate breadcrumbs survived reset
constexpr uint32_t FAULT_MAGIC = 0xDEADBEEF;

// Fault breadcrumb — written to .noinit RAM by the fault handler,
// read on next boot to diagnose the crash.
struct FaultBreadcrumb {
    uint32_t magic;     // FAULT_MAGIC if valid
    uint32_t pc;        // Program counter at fault
    uint32_t lr;        // Link register (return address)
    uint32_t cfsr;      // Configurable Fault Status Register
    uint32_t hfsr;      // Hard Fault Status Register
    uint32_t mmfar;     // MemManage Fault Address
    uint32_t bfar;      // Bus Fault Address
    uint8_t faultType;  // 0=HardFault, 1=BusFault, 2=UsageFault, 3=MemManage
    uint32_t uptime_ms; // Approximate uptime when fault occurred
};

// Check if a breadcrumb is valid (magic matches)
inline bool faultBreadcrumbValid(const FaultBreadcrumb& fb) {
    return fb.magic == FAULT_MAGIC;
}

// Clear breadcrumb (call after displaying to user)
inline void faultBreadcrumbClear(FaultBreadcrumb& fb) {
    fb.magic = 0;
}

// Format fault breadcrumb for CLI display
// Returns bytes written to buf
int formatFaultDump(const FaultBreadcrumb& fb, char* buf, int bufLen);

// Decode CFSR bits into human-readable flags
// Returns bytes written
int decodeCfsr(uint32_t cfsr, char* buf, int bufLen);

} // namespace melty
