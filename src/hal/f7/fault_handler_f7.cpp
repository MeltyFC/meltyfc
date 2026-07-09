// MeltyFC — Hardware Fault Handler: STM32F7 (A2)
// Same as F4 + D-cache disable before breadcrumb writes.

#ifndef NATIVE_BUILD
#ifdef STM32F7xx

#include "stm32f7xx.h"
#include "hal/common/fault_handler_hw.h"
#include "target.h"

namespace melty {

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
    SCB_DisableDCache();  // F7: disable cache before breadcrumb writes
    faultBreadcrumb.magic = FAULT_BREADCRUMB_MAGIC;
    faultBreadcrumb.pc = stackFrame[6];
    faultBreadcrumb.lr = stackFrame[5];
    faultBreadcrumb.cfsr = SCB->CFSR;
    faultBreadcrumb.hfsr = SCB->HFSR;
    faultBreadcrumb.resetCauseRaw = 0;
    while (1) {}
}

} // namespace melty

extern "C" void HardFault_Handler(void) {
    uint32_t* sp;
    __asm volatile("tst lr, #4\nite eq\nmrseq %0, msp\nmrsne %0, psp" : "=r"(sp));
    melty::faultHandlerKillAndBreadcrumb(sp);
}
extern "C" void BusFault_Handler(void)   { HardFault_Handler(); }
extern "C" void UsageFault_Handler(void) { HardFault_Handler(); }
extern "C" void MemManage_Handler(void)  { HardFault_Handler(); }

#endif
#endif
