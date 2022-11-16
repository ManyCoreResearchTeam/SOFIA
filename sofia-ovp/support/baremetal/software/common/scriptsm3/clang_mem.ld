/* Linker script to configure memory regions.
 * Need modifying for a specific board.
 *   FLASH.ORIGIN: starting address of flash
 *   FLASH.LENGTH: length of flash
 *   RAM.ORIGIN: starting address of RAM bank 0
 *   RAM.LENGTH: length of RAM bank 0
 */

MEMORY
{
  FLASH (rx) : ORIGIN = 0x0,        LENGTH = 0x10000 /* 64K */
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 0xFC00  /* 63K */
  /* 64K is available, but reserve some space for    */
  /*   1) debug tester communication for debug tests */
  /*   2) DMA data structure for DMA tests           */
}
