#ifndef _OPTIONS
#define _OPTIONS

struct optionsS {
    const char*          zimage;
    Uns64                zimageaddr;
    const char*          initrd;
    Uns64                initrdaddr;
    const char*          linuxsym;
    const char*          linuxcmd;
    Int32                boardid;
    Uns64                linuxmem;
    const char*          boot;
    const char*          image0;
    Uns64                image0addr;
    const char*          image0sym;
    const char*          image1;
    Uns64                image1addr;
    const char*          image1sym;
    const char*          uart0port;
    const char*          uart1port;
    Bool                 nographics;
    Bool                 goldrun;
    // FIM options
    Bool                 faultcampaignrun;
    Bool                 enableconsole;
    Bool                 linuxgraphics;
    const char*          arch;
    const char*          application;
    const char*          projectfolder;
    const char*          applicationfolder;
    const char*          mode;
    const char*          faultlist;
    const char*          faulttype;
    const char*          linuxdtb;
    const char*          environment;
    const char*          interceptlib;
    Uns32                platformid;
    Uns32                numberoffaults;
    Uns64                checkpointinterval;
    Uns32                targetcpucores;
    Uns32                applicationargs;
    // Function Profile
    const char*          tracesymbol;
    const char*          tracevariable;
    Bool                 enablefunctionprofile;
    Bool                 enablelinecoverage;
    Bool                 enableitrace;
    Bool				 enableftrace;
} options = {
    0
};

#endif
