IMPERAS_HOME := $(shell getpath.exe "$(IMPERAS_HOME)")

NOVLNV=1

VERBOSE=1

include $(IMPERAS_HOME)/bin/Makefile.include

ifndef IMPERAS_HOME
  IMPERAS_ERROR := $(error "IMPERAS_HOME not defined")
endif

include $(IMPERAS_HOME)/ImperasLib/buildutils/Makefile.common

#______________________________________________________________________

FimFLAGS = -pthread

SRC=platform.cpp
SUFFIX=$(suffix $(SRC))

ifeq ($(SUFFIX),.c)
  CC=$(HOST_GCC) $(FimFLAGS) $(DEFINES) 
else
  CC=$(HOST_GXX) $(FimFLAGS) $(DEFINES) 
endif

PLATFORM_CFLAGS = $(SIM_CFLAGS)
PLATFORM_LDFLAGS = $(SIM_LDFLAGS) 

EXECUTABLE?=../$(SRC:$(SUFFIX)=.$(IMPERAS_ARCH).exe)
SHAREDOBJ?=$(SRC:$(SUFFIX)=.$(IMPERAS_SHRSUF))
PLATOBJ?=$(WORKDIR)/icmCpuManager.o $(WORKDIR)/FIM.o $(WORKDIR)/OptionParser.o $(WORKDIR)/$(SRC:$(SUFFIX)=.o)

all: clean  $(WORKDIR)/$(SHAREDOBJ) $(WORKDIR)/$(EXECUTABLE) finish

$(WORKDIR)/$(EXECUTABLE): $(PLATOBJ)
	$(V) mkdir -p $(@D)
	@    echo "# Host Linking Platform $@"
	$(V) $(CC) -o $@ $^ $(PLATFORM_LDFLAGS) $(LDFLAGS) $(IMPERAS_ICMSTUBS)
	$(V) # if we are not compiling locally, copy out the .xml files to the destination
ifeq ($(COPYXML),1)
	@    echo "# Copying XML $(XML) to $(@D)"
	$(V) cp $(XML) $(@D)
endif

$(WORKDIR)/$(SHAREDOBJ): $(PLATOBJ)
	$(V) mkdir -p $(@D)
	@    echo "# Host Linking Platform object $@"
	$(V) $(CC) -shared -o $@ $^ $(PLATFORM_LDFLAGS) $(LDFLAGS) $(IMPERAS_ICMSTUBS)

$(WORKDIR)/%.o:%.c
	$(V) mkdir -p $(@D)
	@    echo "# Host Compiling Platform $@"
	$(V) $(CC) -c -o $@ $^ $(PLATFORM_CFLAGS) $(CFLAGS)

$(WORKDIR)/%.o:%.c??
	$(V) mkdir -p $(@D)
	@    echo "# Host Compiling Platform $@"
	$(V) $(CC) -c -o $@ $^ $(PLATFORM_CFLAGS) $(CFLAGS)  

$(WORKDIR)/icmCpuManager.o: $(IMPERAS_HOME)/ImpPublic/source/host/icm/icmCpuManager.cpp $(ICM_API) 
	@    echo "# Compiling icmCpuManager $@"
	$(V) $(CC) -Wno-attributes -fpermissive -w $(PLATFORM_CFLAGS) -c -o icmCpuManager.o ${IMPERAS_HOME}/ImpPublic/source/host/icm/icmCpuManager.cpp 

$(WORKDIR)/FIM.o: FIM.c
	@    echo "# Creating $@"
	$(V) $(CC) -c -o $@ $^ $(PLATFORM_CFLAGS) $(CFLAGS)
	
finish::
	$(V) - rm -f $(WORKDIR)/$(SHAREDOBJ) $(PLATOBJ) *.o
clean::
	$(V) - rm -f $(EXECUTABLE) $(WORKDIR)/$(SHAREDOBJ) $(PLATOBJ) *.a
