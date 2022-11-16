#
# SMP Bootloader for Cortex-A9 processors
#
# Called by the Smart Loader boot code with registers ready for the Linux boot:
#  r0 = 0
#  r1 = board ID
#  r2 = address of ATAGS/Device Tree Blob
#  r3 = Linux entry address
#  sp = top of memory
#
.text
.global _start

_start:
  MRC     p15, 0, r0, c0, c0, 5   @ Read CPU ID register
  ANDS    r0, r0, #0x03           @ Mask off, leaving the CPU ID field

  # IF CPU #0: Jump to Linux entry address in r3
  CMP     r0, #0                  @ IF (ID ==0)
  MOVEQ   pc, r3                  @ branch to address in r3

  # ELSE: Enable GIC and then wait for signal

  # Get the address of the GIC
  MRC     p15, 4, r0, c15, c0, 0  @ Read periph base address      (see DE593076)
  ADD     r0, r0, #0x1000         @ Add GIC offset to base address

  # Enable the GIC
  LDR     r1, [r0]                @ Read the GIC's Enable Register  (ICDDCR)
  ORR     r1, r1, #0x01           @ Set bit 0, the enable bit
  STR     r1, [r0]                @ Write the GIC's Enable Register  (ICDDCR)

  # Enable interrupt source 0->15
  MOV     r1, #0xFF00             @ Load mask
  ORR     r1, #0xFF
  STR     r1, [r0, #0x100]        @ Write enable set register

  # Set priority of interrupt source 0->16 to 0x0 (highest priority)
  MOV     r1, #0x00
  STR     r1, [r0, #0x400]        @ Sources 00-03
  STR     r1, [r0, #0x404]        @ Sources 04-07
  STR     r1, [r0, #0x408]        @ Sources 08-11
  STR     r1, [r0, #0x40C]        @ Sources 12-15

  # Enable the Processor Interface
  MRC     p15, 4, r0, c15, c0, 0  @ Re-Read periph base address
  LDR     r1, [r0, #0x100]        @ Read the Processor Interface Control register
  ORR     r1, r1, #0x03           @ Bit 0: Enables secure interrupts, Bit 1: Enables Non-Secure interrupts
  STR     r1, [r0, #0x100]        @ Write the Processor Interface Control register

  # Set the Processor's Priority Mask
  MOV     r1, #0x1F
  STR     r1, [r0, #0x0104]       @ Write the Priority Mask register

  WFI                             @ Go to sleep until wake-up

  NOP
  NOP
  NOP

  MOV     r1, #0x10000000
  ORR     r1, #0x30
  LDR     r0, [r1]                @ Load address of kernel entry from SYS_FLAGS
  BX      r0
