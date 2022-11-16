IMPERAS_HOME := $(shell getpath.exe "$(IMPERAS_HOME)")

# build locally by default
NOVLNV?=1

all: model.$(IMPERAS_ARCH).$(IMPERAS_SHRSUF)

include $(IMPERAS_HOME)/ImperasLib/buildutils/Makefile.module

model.$(IMPERAS_ARCH).$(IMPERAS_SHRSUF) : model.$(IMPERAS_SHRSUF)
	$(V) mv $^ $@

clean::
	$(V) rm -rf model.$(IMPERAS_ARCH).$(IMPERAS_SHRSUF) objs
