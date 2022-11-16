ENTRY(_Reset)
SECTIONS
{
 . = 0x00000;
 .startup . : { startup.o(.text) }
 .text : { *(.text) }
 
 .data : 
 {
     _data = .;
     *(vtable)
     *(.data)
     _edata = .;
 }
 
 .bss :
 {
     _bss = .;
     __bss_start__ = .;
     *(.bss)
     _ebss = .;
     _end = .;
     __bss_end__ = .;
     __end = .;
     __end__ = .;
     end = .;
 }
 . = ALIGN(8);
 . = . + 0x1000; /* 4kB of stack memory */
 stack_top = .;
}
