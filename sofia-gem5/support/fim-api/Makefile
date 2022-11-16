# Build locally without using a VLNV library
NOVLNV=1
VERBOSE=1


OVP_INCLUDES=-I.
OPTIMISATION?=-O0 -g

ARM_CORTEX_A9_CROSSCOMPILER   = /usr
ARM_CORTEX_A9_PREFIX=arm-linux-gnueabi

ARM_CORTEX_A9_CFLAGS   = -mcpu=cortex-a9 -march=armv7-a -mlittle-endian -static -DUNIX -Wl,-Ttext=0x2FAFAFAF
ARM_CORTEX_A9_CC       = $(ARM_CORTEX_A9_CROSSCOMPILER)/bin/$(ARM_CORTEX_A9_PREFIX)-gcc $(ARM_CORTEX_A9_CFLAGS) -DARM_CORTEX_A9 $(OPTIMISATION) $(OVP_INCLUDES)
ARM_CORTEX_A9_OBJDUMP  = $(ARM_CORTEX_A9_CROSSCOMPILER)/bin/$(ARM_CORTEX_A9_PREFIX)-objdump

ARM_CORTEX_A72_CROSSCOMPILER   = /usr
ARM_CORTEX_A72_PREFIX=aarch64-linux-gnu

ARM_CORTEX_A72_CFLAGS   = -mcpu=cortex-a53 -march=armv8-a -mlittle-endian -static -DUNIX -Wl,-Ttext=0x2FAFAFAF
ARM_CORTEX_A72_CC       = $(ARM_CORTEX_A72_CROSSCOMPILER)/bin/$(ARM_CORTEX_A72_PREFIX)-gcc $(ARM_CORTEX_A72_CFLAGS)  -DARM_CORTEX_A53 $(OPTIMISATION) $(OVP_INCLUDES)
ARM_CORTEX_A72_OBJDUMP  = $(ARM_CORTEX_A72_CROSSCOMPILER)/bin/$(ARM_CORTEX_A72_PREFIX)-objdump

SRC?=OVPFIM-ARMv7.c OVPFIM-ARMv8.c
EXE=$(patsubst %.c,%.elf,$(SRC))

all: $(EXE)
ovparmv8: OVPFIM-ARMv8.elf
ovparmv7: OVPFIM-ARMv7.elf

OVPFIM-ARMv7.elf: OVPFIM-ARMv7.c
	@    echo "# Compiling $<"
	$(V) $(ARM_CORTEX_A9_CC) -o $@ $<  
	$(V) $(ARM_CORTEX_A9_OBJDUMP) -d $@ > app.ARMv7.lst

OVPFIM-ARMv8.elf: OVPFIM-ARMv8.c
	@    echo "# Compiling $<"
	$(V) $(ARM_CORTEX_A72_CC) -o $@ $<  
	$(V) $(ARM_CORTEX_A72_OBJDUMP) -d $@ > app.ARMv8.lst


clean::
	- rm -f *.elf
	
