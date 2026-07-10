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

// R13-2: Naked fault entry — NO compiler prologue/epilogue.
// The two-stage pattern: naked asm selects MSP/PSP, passes frame to C handler.
// Without naked, the compiler may adjust SP before the capture, corrupting the frame.
extern "C" __attribute__((naked)) void HardFault_Handler(void) {
    __asm volatile(
        "tst lr, #4      \n"  // Test bit 2 of EXC_RETURN
        "ite eq           \n"
        "mrseq r0, msp   \n"  // Main stack
        "mrsne r0, psp   \n"  // Process stack
        "b %0            \n"  // Branch to C handler (r0 = frame pointer)
        :: "i"(melty::faultHandlerKillAndBreadcrumb)
    );
}

extern "C" __attribute__((naked)) void BusFault_Handler(void) {
    __asm volatile(
        "tst lr, #4\nite eq\nmrseq r0, msp\nmrsne r0, psp\n"
        "b %0\n" :: "i"(melty::faultHandlerKillAndBreadcrumb)
    );
}
extern "C" __attribute__((naked)) void UsageFault_Handler(void) {
    __asm volatile(
        "tst lr, #4\nite eq\nmrseq r0, msp\nmrsne r0, psp\n"
        "b %0\n" :: "i"(melty::faultHandlerKillAndBreadcrumb)
    );
}
extern "C" __attribute__((naked)) void MemManage_Handler(void) {
    __asm volatile(
        "tst lr, #4\nite eq\nmrseq r0, msp\nmrsne r0, psp\n"
        "b %0\n" :: "i"(melty::faultHandlerKillAndBreadcrumb)
    );
}

#endif // STM32F4xx
#endif // NATIVE_BUILD
