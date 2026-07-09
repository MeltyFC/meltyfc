// MeltyFC — Hardware Fault Handler (A2)
// Overrides HardFault_Handler (+ BusFault, UsageFault, MemManage).
// On fault: kill all motor timer outputs, write breadcrumbs, hang for IWDG.
//
// The motor kill list IS the route table — iterate it and disable each
// used timer's outputs at register level (CCER clear + BDTR MOE clear).
// No HAL calls in the fault handler — register-level only.
//
// Breadcrumbs go in .noinit RAM — survive reset. Boot reads them.
//
// Per-family implementation required (register addresses differ).
// This header defines the common interface.

#pragma once

#include <cstdint>

namespace melty {

// Breadcrumb structure — lives in .noinit section
struct FaultBreadcrumbHw {
    uint32_t magic;         // 0xDEADBEEF if valid
    uint32_t pc;            // Program counter at fault
    uint32_t lr;            // Link register
    uint32_t cfsr;          // Configurable Fault Status Register
    uint32_t hfsr;          // Hard Fault Status Register
    uint32_t resetCauseRaw; // RCC_CSR at boot (paired with C4 reset forensics)
};

static constexpr uint32_t FAULT_BREADCRUMB_MAGIC = 0xDEADBEEFU;

// Placed in .noinit — survives reset
// Declared extern so both the fault handler and boot code can access it
extern FaultBreadcrumbHw __attribute__((section(".noinit"))) faultBreadcrumb;

// Called at boot to check for breadcrumbs from a previous fault
bool faultBreadcrumbValid();

// Clear the breadcrumb (after reading/logging it)
void faultBreadcrumbClear();

// The actual fault handler — called from the HardFault_Handler override.
// 1. Disable all motor timer outputs (register-level, from route table)
// 2. Disable D-cache if M7 (paranoia before breadcrumb writes)
// 3. Write breadcrumbs (PC/LR/CFSR/HFSR from the stacked frame)
// 4. Infinite loop — IWDG will reset
//
// This function MUST NOT:
//   - Allocate memory
//   - Call any HAL function
//   - Use printf/snprintf
//   - Touch the stack beyond what's already there
void faultHandlerKillAndBreadcrumb(uint32_t* stackFrame);

} // namespace melty
