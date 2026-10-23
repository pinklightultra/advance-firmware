/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2025 STMicroelectronics.
 * Copyright (c) 2025 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM); /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x200;      /* required amount of heap  */
_Min_Stack_Size = 0x400; /* required amount of stack */

/* Specify the memory areas */
MEMORY
{
  DTCMRAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 128K
  RAM (xrw)      : ORIGIN = 0x24000000, LENGTH = 512K
  RAM_D2 (xrw)   : ORIGIN = 0x30000000, LENGTH = 288K
  RAM_D3 (xrw)   : ORIGIN = 0x38000000, LENGTH = 64K
  ITCMRAM (xrw)  : ORIGIN = 0x00000000, LENGTH = 64K
  /* FLASH size: 15 sectors of spanning BANK1 and BANK2, 16th sector is bootloader */
  FLASH (rx)     : ORIGIN = 0x8000000, LENGTH = 1920K
  /* Lower 16 MB of SDRAM1 reserved for sample storage so it can be mapped as a
     single cacheable MPU region. Upper 16 MB stays available for non-cacheable
     display/DMA buffers. */
  SDRAM1 (xrw)   : ORIGIN = 0xC0000000, LENGTH = 16M
  RAM_EXT (xrw)  : ORIGIN = 0xC1000000, LENGTH = 8M
  SDRAM1_NC (xrw): ORIGIN = 0xC1800000, LENGTH = 8M
  SDRAM2 (xrw)   : ORIGIN = 0xD0000000, LENGTH = 32M
}

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* Firmware information for release image intended to be loaded from bootloader
     firmware_info field is 16 bytes, record at the end of sector */
  .firmware_info 0x080002A0 : ALIGN(4)
  {
    KEEP(*(.firmware_info))
  } >FLASH

  /* Put fast functions into ITCM */
  _sifastcode = LOADADDR(.fastcode);
  .fastcode :
  {
    . = ALIGN(4);
    _sfastcode = .;
    *(.fastcode)
    *(.fastcode*)
    . = ALIGN(4);
    _efastcode = .;
  } >ITCMRAM AT> FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab (READONLY) :
  {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >FLASH

  .ARM (READONLY) :
  {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } >FLASH

  .preinit_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .init_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .fini_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
    . = ALIGN(4);
  } >FLASH

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    *(.RamFunc)
    *(.RamFunc*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  .DTCMRAM (NOLOAD) :
  {
    *(.DTCMRAM)
    *(.DTCMRAM*)
  } >DTCMRAM

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >DTCMRAM

  .DATA_RAM (NOLOAD) :
  {
    *(.DATA_RAM)
    *(.DATA_RAM.*)
  } >RAM

  .RAM_D2 (NOLOAD) :
  {
    *(.RAM_D2)
    *(.RAM_D2.*)
  } >RAM_D2

  .RAM_D3 (NOLOAD) :
  {
    *(.RAM_D3)
    *(.RAM_D3.*)
  } >RAM_D3

  .RAM_EXT (NOLOAD) :
  {
    . = ALIGN(32);
    *(.RAM_EXT)
    . = ALIGN(32);
  } >RAM_EXT

  .FRAMEBUFFER (NOLOAD) :
  {
    . = ALIGN(32);
    *(.FRAMEBUFFER)
    . = ALIGN(32);
  } >SDRAM1_NC

  .SDRAM1 (NOLOAD) :
  {
    . = ALIGN(32);
    *(.SDRAM1 .SDRAM1.*)
    . = ALIGN(32);
  } >SDRAM1

  .SDRAM1_NC (NOLOAD) :
  {
    . = ALIGN(32);
    *(.SDRAM1_NC .SDRAM1_NC.*)
    . = ALIGN(32);
  } >SDRAM1_NC

  .SDRAM2 (NOLOAD) :
  {
    . = ALIGN(32);
    *(.SDRAM2 .SDRAM2.*)
    . = ALIGN(32);
  } >SDRAM2

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a(*)
    libm.a(*)
    libgcc.a(*)
  }
}
