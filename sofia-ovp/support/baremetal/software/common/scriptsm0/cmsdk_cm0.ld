/*
 *-----------------------------------------------------------------------------
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from ARM Limited.
 *
 *            (C) COPYRIGHT 2010-2015  ARM Limited or its affiliates.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from ARM Limited.
 *
 *      SVN Information
 *
 *      Checked In          : $Date: $
 *
 *      Revision            : $Revision: $
 *
 *      Release Information : Cortex-M0 DesignStart-r1p0-00rel0
 *-----------------------------------------------------------------------------
 */
/* Linker script to configure memory regions.
 * Need modifying for a specific board.
 *   FLASH.ORIGIN: starting address of flash
 *   FLASH.LENGTH: length of flash
 *   RAM.ORIGIN: starting address of RAM bank 0
 *   RAM.LENGTH: length of RAM bank 0
 */

INCLUDE "lib-nosys.ld"

MEMORY
{
  FLASH (rx) : ORIGIN = 0x0,        LENGTH = 0x00030000 /* 64K */
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 0x1000FC00  /* 63K */
  /* 64K is available, but reserve some space for    */
  /*   1) debug tester communication for debug tests */
  /*   2) DMA data structure for DMA tests           */
}

INCLUDE "sections.ld"
