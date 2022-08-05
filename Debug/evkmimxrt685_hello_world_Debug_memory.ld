/*
 * GENERATED FILE - DO NOT EDIT
 * Copyright (c) 2008-2013 Code Red Technologies Ltd,
 * Copyright 2015, 2018-2019 NXP
 * (c) NXP Semiconductors 2013-2022
 * Generated linker script file for MIMXRT685S
 * Created from memory.ldt by FMCreateLinkMemory
 * Using Freemarker v2.3.30
 * MCUXpresso IDE v11.5.1 [Build 7266] [2022-04-13] on Aug 5, 2022, 11:10:17 AM
 */

MEMORY
{
  /* Define each memory region */
  SRAM_ROM (rwx) : ORIGIN = 0x0, LENGTH = 0x180000 /* 1536K bytes (alias RAM) */  
  USB_RAM (rwx) : ORIGIN = 0x40140000, LENGTH = 0x4000 /* 16K bytes (alias RAM2) */  
}

  /* Define a symbol for the top of each memory region */
  __base_SRAM_ROM = 0x0  ; /* SRAM_ROM */  
  __base_RAM = 0x0 ; /* RAM */  
  __top_SRAM_ROM = 0x0 + 0x180000 ; /* 1536K bytes */  
  __top_RAM = 0x0 + 0x180000 ; /* 1536K bytes */  
  __base_USB_RAM = 0x40140000  ; /* USB_RAM */  
  __base_RAM2 = 0x40140000 ; /* RAM2 */  
  __top_USB_RAM = 0x40140000 + 0x4000 ; /* 16K bytes */  
  __top_RAM2 = 0x40140000 + 0x4000 ; /* 16K bytes */  
