# Build locally without using a VLNV library
NOVLNV=1
VERBOSE=1

ARM_TOOLCHAIN_ROOT = $(ARM_TOOLCHAIN_LINUX)
ARM_CORTEX_PREFIX  = $(ARM_TOOLCHAIN_LINUX_PREFIX)

export ARM_CORTEX_A9_LDFLAGS  = $(MAKEFILE_FLINKFLAGS)

export ARM_CORTEX_A9_CFLAGS   = $(MAKEFILE_CFLAGS)
export ARM_CORTEX_A9_AFLAGS   = $(MAKEFILE_CFLAGS)

#ARM_CORTEX_A9_LINKER_SCRIPT?=-T $(ARM_TOOLCHAIN_ROOT)/arm-none-linux-gnueabi/lib/generic-hosted.ld
ARM_CORTEX_A9_LDFLAGS0 += $(ARM_CORTEX_A9_LINKER_SCRIPT)

#Force cross compile
CROSS=ARM_CORTEX_A9

export ARM_CORTEX_A9_GCC      = $(ARM_TOOLCHAIN_ROOT)/bin/$(ARM_CORTEX_PREFIX)-gcc
export ARM_CORTEX_A9_CC       = $(ARM_TOOLCHAIN_ROOT)/bin/$(ARM_CORTEX_PREFIX)-gcc $(ARM_CORTEX_A9_CFLAGS) 
export ARM_CORTEX_A9_CXX      = $(ARM_TOOLCHAIN_ROOT)/bin/$(ARM_CORTEX_PREFIX)-g++ $(ARM_CORTEX_A9_CFLAGS) 
export ARM_CORTEX_A9_LINK     = $(ARM_TOOLCHAIN_ROOT)/bin/$(ARM_CORTEX_PREFIX)-gcc $(ARM_CORTEX_A9_CFLAGS) $(ARM_CORTEX_A9_LDFLAGS0)
export ARM_CORTEX_A9_LINKXX   = $(ARM_TOOLCHAIN_ROOT)/bin/$(ARM_CORTEX_PREFIX)-g++ $(ARM_CORTEX_A9_CFLAGS) $(ARM_CORTEX_A9_LDFLAGS0)
export ARM_CORTEX_A9_AS       = $(ARM_TOOLCHAIN_ROOT)/bin/$(ARM_CORTEX_PREFIX)-as  $(ARM_CORTEX_A9_AFLAGS)
export ARM_CORTEX_A9_LD       = $(ARM_TOOLCHAIN_ROOT)/bin/$(ARM_CORTEX_PREFIX)-ld
export ARM_CORTEX_A9_AR       = $(ARM_TOOLCHAIN_ROOT)/bin/$(ARM_CORTEX_PREFIX)-ar
export ARM_CORTEX_A9_NM       = $(ARM_TOOLCHAIN_ROOT)/bin/$(ARM_CORTEX_PREFIX)-nm
export ARM_CORTEX_A9_RANLIB   = $(ARM_TOOLCHAIN_ROOT)/bin/$(ARM_CORTEX_PREFIX)-ranlib
export ARM_CORTEX_A9_STRIP    = $(ARM_TOOLCHAIN_ROOT)/bin/$(ARM_CORTEX_PREFIX)-strip
export ARM_CORTEX_A9_READELF  = $(ARM_TOOLCHAIN_ROOT)/bin/$(ARM_CORTEX_PREFIX)-readelf
export ARM_CORTEX_A9_OBJDUMP  = $(ARM_TOOLCHAIN_ROOT)/bin/$(ARM_CORTEX_PREFIX)-objdump
export ARM_CORTEX_A9_OBJCOPY  = $(ARM_TOOLCHAIN_ROOT)/bin/$(ARM_CORTEX_PREFIX)-objcopy

#If hava a custom makefile
ifeq ($(shell find -name "makefile.include"),./makefile.include)
include makefile.include
else
#Others
.SUFFIXES: .c .cpp

.PHONY: cleanObj

MULT?=
DEF?=

SRC?=$(shell find . -type f -name '*.c' -o -name '*.cpp' | grep -v "./Data*" | sort)

INCOBJ=$(patsubst %.c,%$(MULT).$(CROSS).o,$(shell find .  -type f -name '*.c' | grep -v "./Data*" | sort )) $(patsubst %.cpp,%$(MULT).$(CROSS).o,$(shell find . -type f -name '*.cpp' | grep -v "./Data*" | sort))
EXE=app.$(CROSS).elf

V=

all: cleanObj print $(EXE) cleanObj 

print::
	@    echo "Print $(INCOBJ) - $(SRC)"
	
$(EXE): $(INCOBJ)
	@    echo "# Linking $(CROSS) $@"
	@    echo $(INCOBJ)
	$(V) $(ARM_CORTEX_A9_LINKXX) -o $@ $^ $(ARM_CORTEX_A9_LDFLAGS) $(ARM_CORTEX_A9_LDFLAGS0)
	$(V) $(ARM_CORTEX_A9_OBJDUMP) -d $@ > app.FIM_log.lst
	$(V) rm -f *.o

%$(MULT).$(CROSS).o: %.cpp
	@    echo "# Compiling cpp $(CROSS) $<"
	$(V) $(ARM_CORTEX_A9_CXX) -c -o $@ $< -D$(CROSS) $(DEF) $(OPTIMISATION)

%$(MULT).$(CROSS).o: %.c
	@    echo "# Compiling $(CROSS) $<"
	$(V) $(ARM_CORTEX_A9_CC) -c -o $@ $< -D$(CROSS) $(DEF) $(OPTIMISATION) 

cleanObj:
	@    echo "# Clean"
	$(V) rm -f *.o

clean:
	- rm -f *.$(CROSS).elf *.$(CROSS).o
	
endif
