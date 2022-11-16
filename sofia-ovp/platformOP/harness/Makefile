IMPERAS_HOME := $(shell getpath.exe "$(IMPERAS_HOME)")

# build locally by default
NOVLNV?=1

include $(IMPERAS_HOME)/ImperasLib/buildutils/Makefile.harness

::clean
	rm -rf *elf obj model.so
