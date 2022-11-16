IMPERAS_HOME := $(shell getpath.exe "$(IMPERAS_HOME)")

# Build locally without using a VLNV library
NOVLNV=1
VERBOSE=1

CFLAGS= $(MAKEFILE_CFLAGS) 

MULT?=
DEF?=

SRC?=$(wildcard *.c)
SRCINC?=

INCOBJ=$(patsubst %.c,%$(MULT).$(CROSS).o,$(SRCINC))
EXE=$(patsubst %.c,%$(MULT).$(CROSS).elf,$(SRC))

-include $(BAREMETAL_FOLDER)/$(CROSS).makefile.include

all: $(EXE)

%$(MULT).$(CROSS).elf: %$(MULT).$(CROSS).o $(INCOBJ)
	@    echo "# Linking $(CROSS) $@"
	$(V) $(IMPERAS_LINK) -o $@ $^ $(IMPERAS_LDFLAGS) -lm -lc  $(CFLAGS)
	$(V) $(IMPERAS_OBJDUMP) -d $@ > app.FIM_log.lst

%$(MULT).$(CROSS).o: %.c
	@    echo "# Compiling $(CROSS) $<"
	@    echo "$(IMPERAS_CC)"
	$(V) $(IMPERAS_CC) -c -o $@ $< -D$(CROSS) $(DEF) $(CFLAGS)

clean::
	- rm -f *.$(CROSS).elf *.$(CROSS).o
	
