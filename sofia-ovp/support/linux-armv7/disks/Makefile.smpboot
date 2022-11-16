IMPERAS_HOME := $(shell getpath.exe "$(IMPERAS_HOME)")
include $(IMPERAS_HOME)/bin/Makefile.include

# Various Cross compiler setups, Default or1k
CROSS?=ARM_CORTEX_A9
-include $(IMPERAS_HOME)/lib/$(IMPERAS_ARCH)/CrossCompiler/$(CROSS).makefile.include
ifeq ($($(CROSS)_CC),)
    IMPERAS_ERROR := $(error "Please install the toolchain to support $(CROSS) ")
endif

ASRC = smpboot.s
OBJ = $(patsubst %.s, %.o, $(ASRC))

APP = smpboot.$(CROSS).elf
all: ${APP}

# Must specify text address that isn't overwritten by smartLoader
${APP}: ${OBJ}
	$(V) echo "Linking $@"
	$(V) $(IMPERAS_LD) -Ttext=0x60001000 -o $@ $^ 

%.o: %.s
	$(V) echo "Assembling $<"
	$(V) $(IMPERAS_AS) $< -o $@

clean:
	$(V) rm -f $(APP)
	$(V) rm -f $(OBJ)
