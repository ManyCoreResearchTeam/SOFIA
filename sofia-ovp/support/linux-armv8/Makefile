# Build locally without using a VLNV library
NOVLNV=1
VERBOSE=1

AARCH64_CROSSCOMPILER   = $(AARCH64_TOOLCHAIN_LINUX)
ARM_CORTEX_PREFIX       = $(AARCH64_TOOLCHAIN_LINUX_PREFIX)


#Flags
export AARCH64_LDFLAGS  = $(MAKEFILE_FLINKFLAGS)
export AARCH64_CFLAGS   = $(MAKEFILE_CFLAGS)
export AARCH64_AFLAGS   = $(MAKEFILE_CFLAGS)

#AARCH64_LINKER_SCRIPT?=-T $(ARM_TOOLCHAIN_ROOT)/arm-none-linux-gnueabi/lib/generic-hosted.ld
AARCH64_LDFLAGS0 += $(AARCH64_LINKER_SCRIPT)

ifndef IMPERAS_HOME
  IMPERAS_ERROR := $(error "IMPERAS_HOME not defined")
endif
ifndef IMPERAS_ARCH
  IMPERAS_ERROR := $(error "IMPERAS_ARCH not defined")
endif

include $(IMPERAS_HOME)/lib/$(IMPERAS_ARCH)/CrossCompiler/Makefile.common.include

AARCH64_GCC      = $(AARCH64_CROSSCOMPILER)/bin/$(ARM_CORTEX_PREFIX)-gcc
AARCH64_CC       = $(AARCH64_CROSSCOMPILER)/bin/$(ARM_CORTEX_PREFIX)-gcc $(AARCH64_CFLAGS) $(AARCH64_INCLUDES)
AARCH64_CXX      = $(AARCH64_CROSSCOMPILER)/bin/$(ARM_CORTEX_PREFIX)-g++ $(AARCH64_CFLAGS) $(AARCH64_INCLUDES)
AARCH64_LINK     = $(AARCH64_CROSSCOMPILER)/bin/$(ARM_CORTEX_PREFIX)-gcc $(AARCH64_CFLAGS) $(AARCH64_INCLUDES) $(AARCH64_LDFLAGS0)
AARCH64_LINKXX   = $(AARCH64_CROSSCOMPILER)/bin/$(ARM_CORTEX_PREFIX)-g++ $(AARCH64_CFLAGS) $(AARCH64_INCLUDES) $(AARCH64_LDFLAGS0)
AARCH64_AS       = $(AARCH64_CROSSCOMPILER)/bin/$(ARM_CORTEX_PREFIX)-as  $(AARCH64_CFLAGS)
AARCH64_LD       = $(AARCH64_CROSSCOMPILER)/bin/$(ARM_CORTEX_PREFIX)-ld
AARCH64_AR       = $(AARCH64_CROSSCOMPILER)/bin/$(ARM_CORTEX_PREFIX)-ar
AARCH64_NM       = $(AARCH64_CROSSCOMPILER)/bin/$(ARM_CORTEX_PREFIX)-nm
AARCH64_RANLIB   = $(AARCH64_CROSSCOMPILER)/bin/$(ARM_CORTEX_PREFIX)-ranlib
AARCH64_STRIP    = $(AARCH64_CROSSCOMPILER)/bin/$(ARM_CORTEX_PREFIX)-strip
AARCH64_READELF  = $(AARCH64_CROSSCOMPILER)/bin/$(ARM_CORTEX_PREFIX)-readelf
AARCH64_OBJDUMP  = $(AARCH64_CROSSCOMPILER)/bin/$(ARM_CORTEX_PREFIX)-objdump
AARCH64_OBJCOPY  = $(AARCH64_CROSSCOMPILER)/bin/$(ARM_CORTEX_PREFIX)-objcopy
AARCH64_GDB      = $(IMPERAS_HOME)/lib/$(IMPERAS_ARCH)/gdb/aarch64-none-elf-gdb

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
	$(V) $(AARCH64_LINK) -o $@ $^ $(AARCH64_LDFLAGS) $(AARCH64_LDFLAGS0)
	$(V) $(AARCH64_OBJDUMP) -d $@ > $@.lst
	$(V) rm -f *.o

%$(MULT).$(CROSS).o: %.cpp
	@    echo "# Compiling cpp $(CROSS) $<"
	$(V) $(AARCH64_CXX) -c -o $@ $< -D$(CROSS) $(DEF)

%$(MULT).$(CROSS).o: %.c
	@    echo "# Compiling $(CROSS) $<"
	$(V) $(AARCH64_CC) -c -o $@ $< -D$(CROSS) $(DEF)

cleanObj:
	@    echo "# Clean"
	$(V) rm -f *.o

clean:
	- rm -f *.$(CROSS).elf *.$(CROSS).o
	
endif
