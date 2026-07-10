// MeltyFC — Hardware Fault Handler: STM32H7 (A2)
// Same as F7 + D-cache disable. H7 DTCM breadcrumbs are CPU-accessible.

#ifndef NATIVE_BUILD
#ifdef STM32H7xx

#include "stm32h7xx.h"
#include "hal/common/fault_handler_hw.h"
#include "target.h"

namespace melty {

// .noinit in DTCM on H7 (CPU-private, survives reset)
FaultBreadcrumbHw __attribute__((section(".noinit"))) faultBreadcrumb;

bool faultBreadcrumbValid() { return faultBreadcrumb.magic == FAULT_BREADCRUMB_MAGIC; }
void faultBreadcrumbClear() { faultBreadcrumb.magic = 0; }

static void killAllMotors() {
    TIM1->CCER = 0;  TIM1->BDTR &= ~TIM_BDTR_MOE;
    TIM3->CCER = 0;
    TIM4->CCER = 0;
    TIM8->CCER = 0;  TIM8->BDTR &= ~TIM_BDTR_MOE;
}

void faultHandlerKillAndBreadcrumb(uint32_t* stackFrame) {
    killAllMotors();
    SCB_DisableDCache();  // H7: disable cache before breadcrumb writes
    faultBreadcrumb.magic = FAULT_BREADCRUMB_MAGIC;
    faultBreadcrumb.pc = stackFrame[6];
    faultBreadcrumb.lr = stackFrame[5];
    faultBreadcrumb.cfsr = SCB->CFSR;
    faultBreadcrumb.hfsr = SCB->HFSR;
    faultBreadcrumb.resetCauseRaw = 0;
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

#endif
#endif
