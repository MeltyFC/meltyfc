// MeltyFC — Hardware Fault Handler: STM32F4 (A2)
// Register-level motor kill + breadcrumb write. No HAL calls.

#ifndef NATIVE_BUILD
#ifdef STM32F4xx

#include "stm32f4xx.h"
#include "hal/common/fault_handler_hw.h"
#include "target.h"

namespace melty {

// .noinit breadcrumb — survives reset
FaultBreadcrumbHw __attribute__((section(".noinit"))) faultBreadcrumb;

bool faultBreadcrumbValid() {
    return faultBreadcrumb.magic == FAULT_BREADCRUMB_MAGIC;
}

void faultBreadcrumbClear() {
    faultBreadcrumb.magic = 0;
}

// Kill all motor timer outputs at register level
// Clears CCER (all channels disabled) and BDTR.MOE (master output off)
static void killAllMotors() {
    // Kill every timer we might use — brute force is fine in a fault handler
    TIM1->CCER = 0;  TIM1->BDTR &= ~TIM_BDTR_MOE;
    TIM3->CCER = 0;
    TIM4->CCER = 0;
#ifdef TIM8
    TIM8->CCER = 0;  TIM8->BDTR &= ~TIM_BDTR_MOE;
#endif
}

void faultHandlerKillAndBreadcrumb(uint32_t* stackFrame) {
    // 1. Kill motors — highest priority
    killAllMotors();

    // 2. Write breadcrumbs from the stacked exception frame
    // Stack frame layout: [R0, R1, R2, R3, R12, LR, PC, xPSR]
    faultBreadcrumb.magic = FAULT_BREADCRUMB_MAGIC;
    faultBreadcrumb.pc = stackFrame[6];   // PC at fault
    faultBreadcrumb.lr = stackFrame[5];   // LR
    faultBreadcrumb.cfsr = SCB->CFSR;
    faultBreadcrumb.hfsr = SCB->HFSR;
    faultBreadcrumb.resetCauseRaw = 0;    // Filled at next boot from RCC_CSR

    // 3. Hang — IWDG will reset us
    while (1) {}
}

} // namespace melty

// Override the weak HardFault_Handler from the startup file
extern "C" void HardFault_Handler(void) {
    // Determine which stack pointer was in use (MSP or PSP)
    uint32_t* sp;
    __asm volatile(
        "tst lr, #4      \n"
        "ite eq           \n"
        "mrseq %0, msp   \n"
        "mrsne %0, psp   \n"
        : "=r"(sp)
    );
    melty::faultHandlerKillAndBreadcrumb(sp);
}

extern "C" void BusFault_Handler(void)   { HardFault_Handler(); }
extern "C" void UsageFault_Handler(void) { HardFault_Handler(); }
extern "C" void MemManage_Handler(void)  { HardFault_Handler(); }

#endif // STM32F4xx
#endif // NATIVE_BUILD
