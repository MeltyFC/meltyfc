/*
 * MeltyFC — Custom Linker Script for MicoAir H743 V2 (STM32H743VI)
 * H7 memory domains:
 *   - DTCM (0x20000000, 128K): CPU-private, DMA CANNOT reach — stack goes here
 *   - AXI SRAM (0x24000000, 512K): .data/.bss
 *   - D2 SRAM (0x30000000, 128K): .d2_dma — ALL DMA buffers (I-11b)
 *   - D3 SRAM (0x38000000, 64K): BDMA-accessible, unused in v1
 *   - ITCM (0x00000000, 64K): instruction TCM, unused
 *
 * MPU non-cacheable region over D2_SRAM configured at boot (system_h7.cpp)
 */

ENTRY(Reset_Handler)
_estack = ORIGIN(DTCM) + LENGTH(DTCM);  /* Stack in DTCM (fastest, CPU-only) */
_Min_Heap_Size = 0x200;
_Min_Stack_Size = 0x1000;

MEMORY
{
  DTCM     (xrw)  : ORIGIN = 0x20000000, LENGTH = 128K   /* Stack — CPU-private, no DMA! */
  AXI_SRAM (xrw)  : ORIGIN = 0x24000000, LENGTH = 512K   /* .data/.bss */
  D2_SRAM  (xrw)  : ORIGIN = 0x30000000, LENGTH = 128K   /* .d2_dma — DMA buffers (I-11b) */
  FLASH    (rx)   : ORIGIN = 0x08000000, LENGTH = 2048K
}

SECTIONS
{
  .isr_vector : { . = ALIGN(4); KEEP(*(.isr_vector)); . = ALIGN(4); } >FLASH
  .text : { . = ALIGN(4); *(.text) *(.text*) *(.glue_7) *(.glue_7t) *(.eh_frame) KEEP(*(.init)) KEEP(*(.fini)) . = ALIGN(4); _etext = .; } >FLASH
  .rodata : { . = ALIGN(4); *(.rodata) *(.rodata*) . = ALIGN(4); } >FLASH
  .ARM.extab : { . = ALIGN(4); *(.ARM.extab* .gnu.linkonce.armextab.*) . = ALIGN(4); } >FLASH
  .ARM : { . = ALIGN(4); __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; . = ALIGN(4); } >FLASH
  .preinit_array : { . = ALIGN(4); PROVIDE_HIDDEN(__preinit_array_start = .); KEEP(*(.preinit_array*)) PROVIDE_HIDDEN(__preinit_array_end = .); . = ALIGN(4); } >FLASH
  .init_array : { . = ALIGN(4); PROVIDE_HIDDEN(__init_array_start = .); KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array*)) PROVIDE_HIDDEN(__init_array_end = .); . = ALIGN(4); } >FLASH
  .fini_array : { . = ALIGN(4); PROVIDE_HIDDEN(__fini_array_start = .); KEEP(*(SORT(.fini_array.*))) KEEP(*(.fini_array*)) PROVIDE_HIDDEN(__fini_array_end = .); . = ALIGN(4); } >FLASH

  _sidata = LOADADDR(.data);
  .data : { . = ALIGN(4); _sdata = .; *(.data) *(.data*) *(.RamFunc) *(.RamFunc*) . = ALIGN(4); _edata = .; } >AXI_SRAM AT> FLASH
  .bss : { . = ALIGN(4); _sbss = .; __bss_start__ = _sbss; *(.bss) *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .; __bss_end__ = _ebss; } >AXI_SRAM

  /* DMA buffers — MUST be in D2 SRAM (DMA1/DMA2 reachable, MPU non-cacheable) */
  .d2_dma (NOLOAD) : {
    . = ALIGN(4);
    _sd2_dma = .;
    *(.d2_dma)
    *(.d2_dma*)
    . = ALIGN(4);
    _ed2_dma = .;
  } >D2_SRAM

  /* .noinit — survives reset for fault breadcrumbs + reset-cause pairing */
  .noinit (NOLOAD) : {
    . = ALIGN(4);
    _snoinit = .;
    *(.noinit)
    *(.noinit*)
    . = ALIGN(4);
    _enoinit = .;
  } >RAM

  ._user_heap_stack : { . = ALIGN(8); PROVIDE(end = .); PROVIDE(_end = .); . = . + _Min_Heap_Size; . = . + _Min_Stack_Size; . = ALIGN(8); } >AXI_SRAM

  /DISCARD/ : { libc.a(*) libm.a(*) libgcc.a(*) }
  .ARM.attributes 0 : { *(.ARM.attributes) }
}
