#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "icm/icmCpuManager.hpp"
#include <sys/time.h>

#include "FIM.h"


//~ #define SIM_ATTRS (icmNewProcAttrs) (ICM_ATTR_DEFAULT|ICM_ATTR_SIMEX|ICM_ATTR_TRACE)
#define SIM_ATTRS (icmNewProcAttrs) (ICM_ATTR_DEFAULT|ICM_ATTR_SIMEX)
#define SIM_ATTRS_DBG (icmNewProcAttrs) (ICM_ATTR_DEFAULT|ICM_ATTR_SIMEX|ICM_ATTR_TRACE)

// software application to be loaded and executed
//~ void parseArgs(int argc, char **argv);
char modelVariant[STRING_SIZE];
char modelType[STRING_SIZE] = "arm";
int loadList = 1;
unsigned int icmAttrs = ICM_VERBOSE | ICM_STOP_ON_CTRLC;

// temporary string array used to create component labels
// We are using full paths for all files, and the size should be enough
char nameString[512]; 

//Global Variable
icmProcessorP *processors;
icmBusP *bus;
icmPseP *pse;
icmMemoryP *mem;
icmWatchPointP *pccb;
icmNetP *lockup_intr;
const char *model;
const char *semihosting;
int stepIndex;

//Linux  Platfomr Variables
Uns32 uartPort = 5646;
Bool connectUart = false;
Bool enableConsole = false;
Bool noGraphics = false;
char bootCode[256] = "";
Bool loadBootCode = false;
Bool android = false;
char sdCardImage[256] = "";
Bool loadSDCard       = false;
Bool wallclock        = false;
#define WALLCLOCKFACTOR 3.0
#define TIME_SLICE 0.001

//Parser Options
extern optparse::Values options;

#include <dlfcn.h> 
void* lib_handle;       /* handle of the opened library */



using namespace std;

static ICM_SMP_ITERATOR_FN(setStartAddress) {
    Uns32 start = 0x60000000;
    icmMessage("I", "STARTUP", "Set start address for %s to 0x%08x", icmGetProcessorName(processor, ""), start);
    icmSetPC(processor, start);
}

void BareMetalPlaform () {
        ///create a user attribute object
        for (stepIndex=0; stepIndex < NumberOfRunningFaults; stepIndex++) {
        icmAttrListP icmAttr = icmNewAttrList();
        
            if(!options["FIM_ARCH"].compare("ARM_CORTEX_M4") || !options["FIM_ARCH"].compare("ARM_CORTEX_M3")) {
                model = icmGetVlnvString(NULL, "arm.ovpworld.org", "processor", "armm", "1.0", "model");
                icmAddStringAttr(icmAttr, "compatibility", "nopBKPT");
                semihosting = icmGetVlnvString(NULL, "arm.ovpworld.org", "semihosting", "armNewlib", "1.0", "model");
            } else if(!options["FIM_ARCH"].compare("MIPS")){
                model       = icmGetVlnvString(NULL, "mips.ovpworld.org", "processor", "mips32", "1.0", "model");
                semihosting = icmGetVlnvString(NULL, "mips.ovpworld.org", "semihosting", "mips32SDE", "1.0", "model");
                strcpy(modelType, "mips32");
            } else {
                model = icmGetVlnvString(NULL, "arm.ovpworld.org", "processor", "arm", "1.0", "model");
                icmAddStringAttr(icmAttr, "compatibility", "nopSVC");
                semihosting = icmGetVlnvString(NULL, "arm.ovpworld.org", "semihosting", "armNewlib", "1.0", "model");
            }
            
            if(!options["FIM_ARCH"].compare("MIPS")){
                icmAddStringAttr(icmAttr, "cacheIndexBypassTLB","true");
            } else {
                icmAddStringAttr(icmAttr, "endian",        "little");
                icmAddStringAttr(icmAttr, "UAL",                "1");
            }
            
            icmPrintf("Model %s\n",modelVariant);
            icmAddStringAttr(icmAttr, "variant",   modelVariant);
            icmAddDoubleAttr(icmAttr, "mips",             100.0);
            
            ///Set the simulation time slice size
            icmSetSimulationTimeSliceDouble(TIME_SLICE);
            
            ///Create the PE name
            sprintf(nameString, "%d", stepIndex);
            
            ///Create a new processor instance
            processors[stepIndex] = icmNewProcessor(
                nameString,         // CPU name
                modelType,          // CPU type
                0,                  // CPU cpuId
                0,                  // CPU model flags
                32,                 // address bits
                model,              // model file
                "modelAttrs",       // morpher attributes
                SIM_ATTRS,          // attributes
                icmAttr,            // user-defined attributes
                semihosting,        // semi-hosting file
                "modelAttrs"        // semi-hosting attributes
            );

            /// Create the processor bus
            sprintf(nameString, "BUS-%d", stepIndex);
            bus[stepIndex] = icmNewBus(nameString, 32);
            
            /// Connect the processors onto the busses
            icmConnectProcessorBusses(processors[stepIndex], bus[stepIndex], bus[stepIndex]);
            
            /// Create the memory
            sprintf(nameString, "MEM-%d", stepIndex);
            mem[stepIndex] = icmNewMemory(nameString, ICM_PRIV_RWX, SIZE_MEMORY);
            
            /// Connect the memory onto the busses
            icmConnectMemoryToBus(bus[stepIndex], "mp1", mem[stepIndex], BASE_MEMORY);
                
                ///Extra memory to cover the 0XFFFFFFFF-4 cause by the first push
                /// Create the memory
                sprintf(nameString, "MEM2-%d", stepIndex);
                icmMemoryP mem2 = icmNewMemory(nameString, ICM_PRIV_RWX, 0XFFF);
                
                /// Connect the memory onto the busses
                icmConnectMemoryToBus(bus[stepIndex], "mp1", mem2, 0XFFFFFFFF - 0XFFF);


            ///Load application binary
            icmLoadProcessorMemory(processors[stepIndex], options["FIM_Application"].c_str(), (icmLoaderAttrs) 0 , 1 , 1);
            
            ///Initialize FIM
            initProcessorInstFIM(stepIndex,processors[stepIndex],mem[stepIndex]);
            
            ///External Memory
            sprintf(nameString, "regs_falt-%d", stepIndex);
            icmMemoryP regs_falt = icmNewMemory(nameString, ICM_PRIV_RWX,  len);
            icmConnectMemoryToBus( bus[stepIndex], "sp1", regs_falt, baseAddress);

            icmAddReadCallback(processors[stepIndex],baseAddress,baseAddress+len, faultInjector,(void*) ((intptr_t) stepIndex));
            //~ icmAddFetchCallback(processors[stepIndex],baseAddress,baseAddress+len, faultInjector,(void*) stepIndex);
            ///OVP-FIM configuration
            if(IsGoldExecution) {
                //Set Gold Profiling
                setProfiling(stepIndex);
            } else {
                if(!options["FIM_ARCH"].compare("ARM_CORTEX_M4") || !options["FIM_ARCH"].compare("ARM_CORTEX_M3")) {
                    ///ARM M4F LOCKUP EXCEPTION HANDLER
                    sprintf(nameString, "LOCKUP_INT-%d", stepIndex);
                    lockup_intr[stepIndex] = icmNewNet(nameString);
                    
                    icmConnectProcessorNet(processors[stepIndex], lockup_intr[stepIndex], "lockup", ICM_OUTPUT);
                    icmAddNetCallback(lockup_intr[stepIndex], LockupExcepetion, (void*) ((intptr_t) stepIndex));
                }
            }
        }/// end of platform creation
}

void LinuxPlatform() {
    
        if (options["FIM_linux_bootloader"].size()!=0) {
            strcpy(bootCode,options["FIM_linux_bootloader"].c_str());
            loadBootCode = true; 
        }
        
        if (!options["FIM_linux_graphics"].compare("nographics")) 
            noGraphics = true;
            
        if (options["FIM_enable_console"].size()!=0) {
            enableConsole = true; 
            connectUart = true;
        }
        //~ connectUart = true;
        for (stepIndex=0; stepIndex < NumberOfRunningFaults; stepIndex++) {
        ////////////////////////////////////////////////////////////////////////////////

            if(wallclock) {
                icmSetWallClockFactor(WALLCLOCKFACTOR);
            }

        ////////////////////////////////////////////////////////////////////////////////
        //                                 Bus smbus_b
        ////////////////////////////////////////////////////////////////////////////////

            sprintf(nameString, "smbus_b-%d", stepIndex);
            icmBusP smbus_b = icmNewBus(nameString, 32);


        ////////////////////////////////////////////////////////////////////////////////
        //                                Bus ddr2bus_b
        ////////////////////////////////////////////////////////////////////////////////

            sprintf(nameString, "ddr2bus_b-%d", stepIndex);
            icmBusP ddr2bus_b = icmNewBus( nameString, 32);


        ////////////////////////////////////////////////////////////////////////////////
        //                               Processor cpu0
        ////////////////////////////////////////////////////////////////////////////////

            model       = icmGetVlnvString(NULL, "arm.ovpworld.org", "processor", "arm", "1.0", "model");
            semihosting = icmGetVlnvString(NULL, "arm.ovpworld.org", "semihosting", "armNewlib", "1.0", "model");


            icmAttrListP cpu_attrList = icmNewAttrList();
            icmAddStringAttr(cpu_attrList, "variant", modelVariant);
            icmAddStringAttr(cpu_attrList, "compatibility", "ISA");
            icmAddUns32Attr(cpu_attrList, "UAL", 1);
            icmAddUns32Attr(cpu_attrList, "showHiddenRegs", 0);
            icmAddUns32Attr(cpu_attrList, "override_CBAR", 0x1e000000);
            icmAddDoubleAttr(cpu_attrList, "mips", 448.000000);
            icmAddStringAttr(cpu_attrList, "endian", "little");
            icmNewProcAttrs cpu_attrs = ICM_ATTR_SIMEX;

            // Perform platform creation and application simulation using OVPsim
            //~ icmSetSimulationTimeSliceDouble(TIME_SLICE);

                sprintf(nameString, "%d", stepIndex);
                processors[stepIndex] = icmNewProcessor(
                    nameString,         // CPU name
                    "arm",              // CPU type
                    stepIndex,          // CPU cpuId
                    0,                  // CPU model flags
                    32,                 // address bits
                    model,              // model file
                    0,                  // morpher attributes
                    cpu_attrs,          // attributes
                    cpu_attrList,           // user-defined attributes
                    semihosting,        // semi-hosting file
                    0                   // semi-hosting attributes
                );

            //icmWriteReg(processors[stepIndex], "ACTLR", 1107296256);
            icmConnectProcessorBusses( processors[stepIndex], smbus_b, smbus_b );

        ////////////////////////////////////////////////////////////////////////////////
        //                                 PSE sysRegs
        ////////////////////////////////////////////////////////////////////////////////

            const char *sysRegs_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "VexpressSysRegs",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP sysRegs_attr = icmNewAttrList();
            icmAddUns32Attr(sysRegs_attr, "SYS_PROCID0", 0x0c000191);
            sprintf(nameString, "sysRegs-%d", stepIndex);

            icmPseP sysRegs_p = icmNewPSE(
                nameString,   // name
                sysRegs_path,   // model
                sysRegs_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( sysRegs_p, smbus_b, "bport1", 0, 0x10000000, 0x10000fff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                 PSE sysCtrl
        ////////////////////////////////////////////////////////////////////////////////

            const char *sysCtrl_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "SysCtrlSP810",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP sysCtrl_attr = icmNewAttrList();
            sprintf(nameString, "sysCtrl-%d", stepIndex);

            icmPseP sysCtrl_p = icmNewPSE(
                nameString,   // name
                sysCtrl_path,   // model
                sysCtrl_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( sysCtrl_p, smbus_b, "bport1", 0, 0x10001000, 0x10001fff);

        ////////////////////////////////////////////////////////////////////////////////
        //                                 PSE sbpci0
        ////////////////////////////////////////////////////////////////////////////////
        
        
            const char *sbpci0_path = icmGetVlnvString(
                0                   ,    // path (0 if from the product directory)
                "ovpworld.org"      ,    // vendor
                0                   ,    // library
                "dummyPort"         ,    // name
                0                   ,    // version
                "pse"                    // model
            );
        
            icmAttrListP sbpci0_attrList = icmNewAttrList();
            sprintf(nameString, "sbpci0-%d", stepIndex);
        
            icmPseP sbpci0_p = icmNewPSE(
                nameString            ,   // name
                sbpci0_path         ,   // model
                sbpci0_attrList     ,   // attrlist
                0,       // unused
                0        // unused
            );
        
            icmConnectPSEBus( sbpci0_p, smbus_b, "bport1", 0, 0x10001000, 0x10001fff);
        ////////////////////////////////////////////////////////////////////////////////
        //                                  PSE aac1
        ////////////////////////////////////////////////////////////////////////////////

            const char *aac1_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "AaciPL041",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP aac1_attr = icmNewAttrList();

            sprintf(nameString, "accc1-%d", stepIndex);

            icmPseP aac1_p = icmNewPSE(
                nameString,   // name
                aac1_path,   // model
                aac1_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( aac1_p, smbus_b, "bport1", 0, 0x10004000, 0x10004fff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                  PSE mmc1
        ////////////////////////////////////////////////////////////////////////////////

            const char *mmc1_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "MmciPL181",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP mmc1_attr = icmNewAttrList();
            sprintf(nameString, "mmc1-%d", stepIndex);

            icmPseP mmc1_p = icmNewPSE(
                nameString,   // name
                mmc1_path,   // model
                mmc1_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );


            icmConnectPSEBus( mmc1_p, smbus_b, "bport1", 0, 0x10005000, 0x10005fff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                   PSE kb1
        ////////////////////////////////////////////////////////////////////////////////

            const char *kb1_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "KbPL050",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP kb1_attr = icmNewAttrList();
            icmAddUns32Attr(kb1_attr, "isKeyboard", 1);
            icmAddUns32Attr(kb1_attr, "grabDisable", 1);
            sprintf(nameString, "kb1-%d", stepIndex);

            icmPseP kb1_p = icmNewPSE(
                nameString,   // name
                kb1_path,   // model
                kb1_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( kb1_p, smbus_b, "bport1", 0, 0x10006000, 0x10006fff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                   PSE ms1
        ////////////////////////////////////////////////////////////////////////////////

            const char *ms1_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "KbPL050",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP ms1_attr = icmNewAttrList();
            icmAddUns32Attr(ms1_attr, "isMouse", 1);
            icmAddUns32Attr(ms1_attr, "grabDisable", 1);

            sprintf(nameString, "ms1-%d", stepIndex);

            icmPseP ms1_p = icmNewPSE(
                nameString,   // name
                ms1_path,   // model
                ms1_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( ms1_p, smbus_b, "bport1", 0, 0x10007000, 0x10007fff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                  PSE uart0
        ////////////////////////////////////////////////////////////////////////////////

            const char *uart0_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "UartPL011",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP uart0_attr = icmNewAttrList();
            icmAddStringAttr(uart0_attr, "variant", "ARM");
            
            sprintf(nameString, "./PlatformLogs/uart%d-%d.log",PlatformIdentifier,stepIndex);
            
            icmAddStringAttr(uart0_attr, "outfile", nameString);
            if(connectUart) {
                if(enableConsole) {
                    icmAddUns64Attr(uart0_attr, "console", 1);
                } else {
                    icmAddUns64Attr(uart0_attr, "portnum", uartPort);
                }
                icmAddUns64Attr(uart0_attr, "finishOnDisconnect", 1);
            }

            sprintf(nameString, "uart0-%d", stepIndex);

            icmPseP uart0_p = icmNewPSE(
                nameString,   // name
                uart0_path,   // model
                uart0_attr,   // attrlist
                NULL,   // semihost file
                NULL    // semihost symbol
            );

            icmConnectPSEBus( uart0_p, smbus_b, "bport1", 0, 0x10009000, 0x10009fff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                  PSE uart1
        ////////////////////////////////////////////////////////////////////////////////

            const char *uart1_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "UartPL011",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP uart1_attr = icmNewAttrList();

            icmAddStringAttr(uart1_attr, "variant", "ARM");
            if(connectUart && android) {
                if(enableConsole) {
                    icmAddUns64Attr(uart1_attr, "console", 1);
                } else {
                    icmAddUns64Attr(uart1_attr, "portnum", (uartPort == 0) ? uartPort : uartPort+1);
                }
                icmAddUns64Attr(uart1_attr, "finishOnDisconnect", 1);
            } else {
                //icmAddStringAttr(uart1_attr, "outfile", "uart1.log");
            }

            sprintf(nameString, "uart1-%d", stepIndex);

            icmPseP uart1_p = icmNewPSE(
                nameString,   // name
                uart1_path,   // model
                uart1_attr,   // attrlist
                NULL,   // semihost file
                NULL    // semihost symbol
            );

            icmConnectPSEBus( uart1_p, smbus_b, "bport1", 0, 0x1000a000, 0x1000afff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                  PSE uart2
        ////////////////////////////////////////////////////////////////////////////////

            const char *uart2_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "UartPL011",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP uart2_attr = icmNewAttrList();

            icmAddStringAttr(uart2_attr, "variant", "ARM");

            sprintf(nameString, "uart2-%d", stepIndex);
            icmPseP uart2_p = icmNewPSE(
                nameString,   // name
                uart2_path,   // model
                uart2_attr,   // attrlist
                NULL,   // semihost file
                NULL    // semihost symbol
            );

            icmConnectPSEBus( uart2_p, smbus_b, "bport1", 0, 0x1000b000, 0x1000bfff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                  PSE uart3
        ////////////////////////////////////////////////////////////////////////////////

            const char *uart3_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "UartPL011",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP uart3_attr = icmNewAttrList();

            icmAddStringAttr(uart3_attr, "variant", "ARM");

            sprintf(nameString, "uart3-%d", stepIndex);
            icmPseP uart3_p = icmNewPSE(
                nameString,   // name
                uart3_path,   // model
                uart3_attr,   // attrlist
                NULL,   // semihost file
                NULL    // semihost symbol
            );

            icmConnectPSEBus( uart3_p, smbus_b, "bport1", 0, 0x1000c000, 0x1000cfff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                  PSE wdt1
        ////////////////////////////////////////////////////////////////////////////////

            const char *wdt1_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "WdtSP805",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP wdt1_attr = icmNewAttrList();

            sprintf(nameString, "wdt1-%d", stepIndex);
            icmPseP wdt1_p = icmNewPSE(
                nameString,   // name
                wdt1_path,   // model
                wdt1_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( wdt1_p, smbus_b, "bport1", 0, 0x1000f000, 0x1000ffff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                 PSE timer01
        ////////////////////////////////////////////////////////////////////////////////

            const char *timer01_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "TimerSP804",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP timer01_attr = icmNewAttrList();

            sprintf(nameString, "timer01-%d", stepIndex);
            icmPseP timer01_p = icmNewPSE(
                nameString,   // name
                timer01_path,   // model
                timer01_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( timer01_p, smbus_b, "bport1", 0, 0x10011000, 0x10011fff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                 PSE timer23
        ////////////////////////////////////////////////////////////////////////////////

            const char *timer23_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "TimerSP804",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP timer23_attr = icmNewAttrList();

            sprintf(nameString, "timer23-%d", stepIndex);

            icmPseP timer23_p = icmNewPSE(
                nameString,   // name
                timer23_path,   // model
                timer23_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( timer23_p, smbus_b, "bport1", 0, 0x10012000, 0x10012fff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                  PSE dvi1
        ////////////////////////////////////////////////////////////////////////////////

            const char *dvi1_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "SerBusDviRegs",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP dvi1_attr = icmNewAttrList();

            sprintf(nameString, "dvi1-%d", stepIndex);

            icmPseP dvi1_p = icmNewPSE(
                nameString,   // name
                dvi1_path,   // model
                dvi1_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( dvi1_p, smbus_b, "bport1", 0, 0x10016000, 0x10016fff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                  PSE rtc1
        ////////////////////////////////////////////////////////////////////////////////

            const char *rtc1_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "RtcPL031",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP rtc1_attr = icmNewAttrList();

            sprintf(nameString, "rtc1-%d", stepIndex);

            icmPseP rtc1_p = icmNewPSE(
                nameString,   // name
                rtc1_path,   // model
                rtc1_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( rtc1_p, smbus_b, "bport1", 0, 0x10017000, 0x10017fff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                   PSE cf1
        ////////////////////////////////////////////////////////////////////////////////

            const char *cf1_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "CompactFlashRegs",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP cf1_attr = icmNewAttrList();

            sprintf(nameString, "cf1-%d", stepIndex);
            icmPseP cf1_p = icmNewPSE(
                nameString,   // name
                cf1_path,   // model
                cf1_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( cf1_p, smbus_b, "bport1", 0, 0x1001a000, 0x1001afff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                  PSE lcd1
        ////////////////////////////////////////////////////////////////////////////////

            const char *lcd1_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "LcdPL110",    // name
                0,    // version
                "pse"     // model
            );

            const char *lcd1_pe = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "LcdPL110",    // name
                0,    // version
                "model"     // model
            );

            icmAttrListP lcd1_attr = icmNewAttrList();

        //    icmAddUns64Attr(lcd1_attr, "scanDelay", 50000);
            icmAddUns64Attr(lcd1_attr, "noGraphics", noGraphics);
            icmAddStringAttr(lcd1_attr, "resolution", "xga");

            sprintf(nameString, "lcd1-%d", stepIndex);
            icmPseP lcd1_p = icmNewPSE(
                nameString,   // name
                lcd1_path,   // model
                lcd1_attr,   // attrlist
                lcd1_pe,   // semihost file
                "modelAttrs"    // semihost symbol
            );

            icmConnectPSEBus( lcd1_p, smbus_b, "bport1", 0, 0x10020000, 0x10020fff);

            icmConnectPSEBusDynamic( lcd1_p, smbus_b, "memory", 0);


        ////////////////////////////////////////////////////////////////////////////////
        //                                    lcd2
        ////////////////////////////////////////////////////////////////////////////////


            // Versatile Express contains CLCD controller on both the motherboard and
            // daughterboard.  Rather than 2 instances, map motherboard controller as RAM
            sprintf(nameString, "lcd2_m-%d", stepIndex);

            icmMemoryP lcd2_m = icmNewMemory(nameString, ICM_PRIV_RWX, 0xfff);
            icmConnectMemoryToBus( smbus_b, "sp1", lcd2_m, 0x1001f000);


        ////////////////////////////////////////////////////////////////////////////////
        //                                  PSE dmc1
        ////////////////////////////////////////////////////////////////////////////////

            const char *dmc1_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "DMemCtrlPL341",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP dmc1_attr = icmNewAttrList();

            sprintf(nameString, "dmc1-%d", stepIndex);
            icmPseP dmc1_p = icmNewPSE(
                nameString,   // name
                dmc1_path,   // model
                dmc1_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( dmc1_p, smbus_b, "bport1", 0, 0x100e0000, 0x100e0fff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                  PSE smc1
        ////////////////////////////////////////////////////////////////////////////////

            const char *smc1_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "SMemCtrlPL354",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP smc1_attr = icmNewAttrList();

            sprintf(nameString, "smc1-%d", stepIndex);

            icmPseP smc1_p = icmNewPSE(
                nameString,   // name
                smc1_path,   // model
                smc1_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( smc1_p, smbus_b, "bport1", 0, 0x100e1000, 0x100e1fff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                 PSE timer45
        ////////////////////////////////////////////////////////////////////////////////

            const char *timer45_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "TimerSP804",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP timer45_attr = icmNewAttrList();

            sprintf(nameString, "timer45-%d", stepIndex);
            icmPseP timer45_p = icmNewPSE(
                nameString,   // name
                timer45_path,   // model
                timer45_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( timer45_p, smbus_b, "bport1", 0, 0x100e4000, 0x100e4fff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                 PSE l2regs
        ////////////////////////////////////////////////////////////////////////////////

            const char *l2regs_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                0,    // vendor
                0,    // library
                "L2CachePL310",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP l2regs_attr = icmNewAttrList();

            sprintf(nameString, "l2regs-%d", stepIndex);
            icmPseP l2regs_p = icmNewPSE(
                nameString,   // name
                l2regs_path,   // model
                l2regs_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( l2regs_p, smbus_b, "bport1", 0, 0x1e00a000, 0x1e00afff);


        ////////////////////////////////////////////////////////////////////////////////
        //                                PSE eth0
        ////////////////////////////////////////////////////////////////////////////////


            const char *eth0_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                "smsc.ovpworld.org",    // vendor
                "peripheral",           // library
                "LAN9118",              // name
                "1.0",                  // version
                "pse"                   // model
            );

            icmAttrListP eth0_attr = icmNewAttrList();

            sprintf(nameString, "eth0-%d", stepIndex);
            icmPseP eth0_p = icmNewPSE(
                nameString,   // name
                eth0_path,   // model
                eth0_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            icmConnectPSEBus( eth0_p, smbus_b, "bport1", 0, 0x4e000000, 0x4e000fff);

        ////////////////////////////////////////////////////////////////////////////////
        //                                PSE usb0
        ////////////////////////////////////////////////////////////////////////////////

            const char *usb0_path = icmGetVlnvString(
                0,    // path (0 if from the product directory)
                "philips.ovpworld.org",    // vendor
                "peripheral",           // library
                "ISP1761",    // name
                0,    // version
                "pse"     // model
            );

            icmAttrListP usb0_attr = icmNewAttrList();

            sprintf(nameString, "usb0-%d", stepIndex);
            icmPseP usb0_p = icmNewPSE(
                nameString,   // name
                usb0_path,   // model
                usb0_attr,   // attrlist
                0,   // semihost file
                0    // semihost symbol
            );

            //~ icmConnectPSEBus( usb0_p, smbus_b, "bport1", 0, 0x4f000000, 0x4f000fff);
            icmConnectPSEBus( usb0_p, smbus_b, "bport1", 0, 0x4f000000, 0x4f00ffff);

           // icmMemoryP m4_m = icmNewMemory("m4_m", 0x7, 0xfff);
           // icmConnectMemoryToBus( smbus_b, "sp1", m4_m, 0x4e000000);

           // icmMemoryP m5_m = icmNewMemory("m5_m", 0x7, 0xfff);
           // icmConnectMemoryToBus( smbus_b, "sp1", m5_m, 0x4f000000);

        ////////////////////////////////////////////////////////////////////////////////
        //                                Memory nor0
        ////////////////////////////////////////////////////////////////////////////////

            // NOR Flash 0
            sprintf(nameString, "nor0_m-%d", stepIndex);
            icmMemoryP nor0_m = icmNewMemory(nameString, ICM_PRIV_RW, 0x03ffffff);
            icmConnectMemoryToBus( smbus_b, "sp1", nor0_m, 0x40000000);

        ////////////////////////////////////////////////////////////////////////////////
        //                                Memory nor1
        ////////////////////////////////////////////////////////////////////////////////

            // NOR Flash 1
            sprintf(nameString, "nor1_m-%d", stepIndex);
            icmMemoryP nor1_m = icmNewMemory(nameString, ICM_PRIV_RW, 0x03ffffff);
            icmConnectMemoryToBus( smbus_b, "sp1", nor1_m, 0x44000000);


        ////////////////////////////////////////////////////////////////////////////////
        //                                Memory sram1
        ////////////////////////////////////////////////////////////////////////////////

            sprintf(nameString, "sram1_m-%d", stepIndex);
            icmMemoryP sram1_m = icmNewMemory(nameString, ICM_PRIV_RWX, 0x1ffffff);
            icmConnectMemoryToBus( smbus_b, "sp1", sram1_m, 0x48000000);


        ////////////////////////////////////////////////////////////////////////////////
        //                                Memory vram1
        ////////////////////////////////////////////////////////////////////////////////

            sprintf(nameString, "vram1_m-%d", stepIndex);
            icmMemoryP vram1_m = icmNewMemory(nameString, ICM_PRIV_RWX, 0x7fffff);

            icmConnectMemoryToBus( smbus_b, "sp1", vram1_m, 0x4c000000);


        ////////////////////////////////////////////////////////////////////////////////
        //                               Memory ddr2ram
        ////////////////////////////////////////////////////////////////////////////////

            sprintf(nameString, "ddr2ram_m-%d", stepIndex);
            icmMemoryP ddr2ram_m = icmNewMemory(nameString, ICM_PRIV_RWX, 0x3fffffff);
            mem[stepIndex] = ddr2ram_m;
            icmConnectMemoryToBus( ddr2bus_b, "sp1", ddr2ram_m, 0x0);


        ////////////////////////////////////////////////////////////////////////////////
        //                            Bridge ddr2RamBridge
        ////////////////////////////////////////////////////////////////////////////////

            sprintf(nameString, "ddr2RamBridge-%d", stepIndex);
            icmNewBusBridge(smbus_b, ddr2bus_b, nameString, "sp", "mp", 0x0, 0x3fffffff, 0x60000000);


        ////////////////////////////////////////////////////////////////////////////////
        //                           Bridge ddr2RemapBridge
        ////////////////////////////////////////////////////////////////////////////////

            sprintf(nameString, "ddr2RemapBridge-%d", stepIndex);
            icmNewBusBridge(smbus_b, ddr2bus_b, nameString, "sp1", "mp", 0x20000000, 0x23ffffff, 0x0);

        ////////////////////////////////////////////////////////////////////////////////
        //                               PSE smartLoader
        ////////////////////////////////////////////////////////////////////////////////

                const char *smartLoader_path = icmGetVlnvString(
                    0,    // path (0 if from the product directory)
                    "arm.ovpworld.org",    // vendor
                    0,    // library
                    "SmartLoaderArmLinux",    // name
                    "1.0",    // version
                    "pse"     // model
                );

                icmAttrListP smartLoader_attr = icmNewAttrList();
                sprintf(nameString, "%s/zImage", LinuxSuportFolder);
                //~ sprintf(nameString, "/home/frdarosa/Documents/zImage");
                icmAddStringAttr(smartLoader_attr, "kernel", nameString);
                if(android) {
                    sprintf(nameString, "%s/initrd.img", LinuxSuportFolder);
                    icmAddStringAttr(smartLoader_attr, "initrd", nameString);
                } else {
                    sprintf(nameString, "./fs.img.FIM_log");
                    icmAddStringAttr(smartLoader_attr, "initrd", nameString);
                }


                char command[256] = "mem=1024M raid=noautodetect console=ttyAMA0,38400n8 vmalloc=256MB devtmpfs.mount=0";
                if(android) {
                    strcat(command, " androidboot.console=ttyAMA0 vga=771");
                }
                icmAddStringAttr(smartLoader_attr, "command", command);
                icmAddUns64Attr(smartLoader_attr, "memsize", (256*1024*1024));
                icmAddUns64Attr(smartLoader_attr, "physicalbase", 0x60000000);
                icmAddUns64Attr(smartLoader_attr, "boardid", 0x8e0); // Versatile Express
                if(loadBootCode ) {
                    icmImagefileP image = icmLoadBus(smbus_b,bootCode,ICM_LOAD_VERBOSE,True);
                    if (image) {
                        icmAddUns64Attr (smartLoader_attr, "bootaddr", icmGetImagefileEntry(image));
                    }
                }

                sprintf(nameString, "smartLoader-%d", stepIndex);
                icmPseP smartLoader_p = icmNewPSE(
                    nameString,   // name
                    smartLoader_path,   // model
                    smartLoader_attr,   // attrlist
                    0,   // semihost file
                    0    // semihost symbol
                );

                icmConnectPSEBus( smartLoader_p, smbus_b, "mport", 1, 0x0, 0xffffffff);

        ////////////////////////////////////////////////////////////////////////////////
        //                                 PSE GPIO0 (dummy)
        ////////////////////////////////////////////////////////////////////////////////


            // Listed as "Reserved" in data sheet, is accessed by Linux gpio device driver
            const char *gpio0_path = icmGetVlnvString(
                0                   ,    // path (0 if from the product directory)
                "ovpworld.org"      ,    // vendor
                0                   ,    // library
                "dummyPort"         ,    // name
                0                   ,    // version
                "pse"                    // model
            );

            icmAttrListP gpio0_attrList = icmNewAttrList();

            sprintf(nameString, "gpio0-%d", stepIndex);
            icmPseP gpio0_p = icmNewPSE(
                nameString             ,   // name
                gpio0_path          ,   // model
                gpio0_attrList      ,   // attrlist
                0                   ,   // semihost file
                0        // unused
            );

            icmConnectPSEBus( gpio0_p, smbus_b, "bport1", 0, 0x100e8000, 0x100e8fff);

        ////////////////////////////////////////////////////////////////////////////////
        //                                 CONNECTIONS
        ////////////////////////////////////////////////////////////////////////////////


            sprintf(nameString, "cardin_n-%d", stepIndex);
            icmNetP cardin_n = icmNewNet(nameString);

            icmConnectPSENet( sysRegs_p, cardin_n, "cardin", ICM_INPUT);

            icmConnectPSENet( mmc1_p, cardin_n, "cardin", ICM_OUTPUT);

            sprintf(nameString, "wprot_n-%d", stepIndex);
            icmNetP wprot_n = icmNewNet(nameString);

            icmConnectPSENet( sysRegs_p, wprot_n, "wprot", ICM_INPUT);

            icmConnectPSENet( mmc1_p, wprot_n, "wprot", ICM_OUTPUT);



            sprintf(nameString, "ir2_n-%d", stepIndex);
            icmNetP ir2_n = icmNewNet(nameString);

            icmConnectProcessorNet( processors[stepIndex], ir2_n, "SPI34", ICM_INPUT);

            icmConnectPSENet( timer01_p, ir2_n, "irq", ICM_OUTPUT);

        ////////////////////////////////////////////////////////////////////////////////

            sprintf(nameString, "ir3_n-%d", stepIndex);
            icmNetP ir3_n = icmNewNet(nameString);

            icmConnectProcessorNet( processors[stepIndex], ir3_n, "SPI35", ICM_INPUT);

            icmConnectPSENet( timer23_p, ir3_n, "irq", ICM_OUTPUT);

        ////////////////////////////////////////////////////////////////////////////////
            sprintf(nameString, "ir4_n-%d", stepIndex);
            icmNetP ir4_n = icmNewNet(nameString);

            icmConnectProcessorNet( processors[stepIndex], ir4_n, "SPI36", ICM_INPUT);

            icmConnectPSENet( rtc1_p, ir4_n, "irq", ICM_OUTPUT);

        ////////////////////////////////////////////////////////////////////////////////
            sprintf(nameString, "ir5_n-%d", stepIndex);
            icmNetP ir5_n = icmNewNet(nameString);

            icmConnectProcessorNet( processors[stepIndex], ir5_n, "SPI37", ICM_INPUT);

            icmConnectPSENet( uart0_p, ir5_n, "irq", ICM_OUTPUT);

        ////////////////////////////////////////////////////////////////////////////////
            sprintf(nameString, "ir6_n-%d", stepIndex);
            icmNetP ir6_n = icmNewNet(nameString);

            icmConnectProcessorNet( processors[stepIndex], ir6_n, "SPI38", ICM_INPUT);

            icmConnectPSENet( uart1_p, ir6_n, "irq", ICM_OUTPUT);

        ////////////////////////////////////////////////////////////////////////////////
            sprintf(nameString, "ir7_n-%d", stepIndex);
            icmNetP ir7_n = icmNewNet(nameString);

            icmConnectProcessorNet( processors[stepIndex], ir7_n, "SPI39", ICM_INPUT);

            icmConnectPSENet( uart2_p, ir7_n, "irq", ICM_OUTPUT);

        ////////////////////////////////////////////////////////////////////////////////
            sprintf(nameString, "ir8_n-%d", stepIndex);
            icmNetP ir8_n = icmNewNet(nameString);

            icmConnectProcessorNet( processors[stepIndex], ir8_n, "SPI40", ICM_INPUT);

            icmConnectPSENet( uart3_p, ir8_n, "irq", ICM_OUTPUT);

        ////////////////////////////////////////////////////////////////////////////////
            sprintf(nameString, "ir9_n-%d", stepIndex);
            icmNetP ir9_n = icmNewNet(nameString);

            icmConnectProcessorNet( processors[stepIndex], ir9_n, "SPI41", ICM_INPUT);

            icmConnectPSENet( mmc1_p, ir9_n, "irq0", ICM_OUTPUT);

        ////////////////////////////////////////////////////////////////////////////////
            sprintf(nameString, "ir10_n-%d", stepIndex);
            icmNetP ir10_n = icmNewNet(nameString);

            icmConnectProcessorNet( processors[stepIndex], ir10_n, "SPI42", ICM_INPUT);

            icmConnectPSENet( mmc1_p, ir10_n, "irq1", ICM_OUTPUT);

        ////////////////////////////////////////////////////////////////////////////////
            sprintf(nameString, "ir12_n-%d", stepIndex);
            icmNetP ir12_n = icmNewNet(nameString);

            icmConnectProcessorNet( processors[stepIndex], ir12_n, "SPI44", ICM_INPUT);

            icmConnectPSENet( kb1_p, ir12_n, "irq", ICM_OUTPUT);

        ////////////////////////////////////////////////////////////////////////////////
            sprintf(nameString, "ir13_n-%d", stepIndex);
            icmNetP ir13_n = icmNewNet(nameString);

            icmConnectProcessorNet( processors[stepIndex], ir13_n, "SPI45", ICM_INPUT);

            icmConnectPSENet( ms1_p, ir13_n, "irq", ICM_OUTPUT);

        ////////////////////////////////////////////////////////////////////////////////
            sprintf(nameString, "ir44_n-%d", stepIndex);
            icmNetP ir44_n = icmNewNet(nameString);

            icmConnectProcessorNet( processors[stepIndex], ir44_n, "SPI76", ICM_INPUT);

            icmConnectPSENet( lcd1_p, ir44_n, "irq", ICM_OUTPUT);

        ////////////////////////////////////////////////////////////////////////////////
            sprintf(nameString, "ir48_n-%d", stepIndex);
            icmNetP ir48_n = icmNewNet(nameString);

            icmConnectProcessorNet( processors[stepIndex], ir48_n, "SPI80", ICM_INPUT);

            icmConnectPSENet( timer45_p, ir48_n, "irq", ICM_OUTPUT);

        ////////////////////////////////////////////////////////////////////////////////

            icmIterAllChildren(processors[stepIndex], setStartAddress, 0);

        ////////////////////////////////////////////////////////////////////////////////
        //                                 Fault injector
        ////////////////////////////////////////////////////////////////////////////////
            ///Extra memory to cover the 0XFFFFFFFF-4 cause by the first push
            /// Create the memory
            sprintf(nameString, "MEM2-FALT%d", stepIndex);
            icmMemoryP mem_falt = icmNewMemory(nameString, ICM_PRIV_RWX, 0XFFF);
            
            /// Connect the memory onto the busses
            icmConnectMemoryToBus(smbus_b, "sp1", mem_falt, 0XCFFFFFFF - 0XFFF);

            sprintf(nameString, "regs_falt-%d", stepIndex);
            icmMemoryP regs_falt = icmNewMemory(nameString, ICM_PRIV_RWX, len);
            icmConnectMemoryToBus( smbus_b, "sp1", regs_falt, baseAddress);

            icmAddReadCallback (processors[stepIndex],baseAddress,baseAddress+len, faultInjector,(void*) ((intptr_t) stepIndex));

            //Initialize Watchdog
            initProcessorInstFIM(stepIndex,processors[stepIndex],ddr2ram_m);
            //Using the other memory
            //~ initProcessorInstFIM(stepIndex,processors[stepIndex],mem_falt);
            
    }
}

//
// Main routine
//
int main(int argc, char **argv) {
    
    ///Parse the arguments
    parseArguments(argc,argv);
    
    printf("Process PID : %d \n",getpid());
    
    ///Add model.so intercept library -- temporary
    //char* dummy_args[] = { "dummy", "--extlib", "FIM/0=/home/frdarosa/Workspace/ovp-fim-16/Platform/InterceptLib/model.so", NULL };
    
    //~ if( options["FIM_application_argv"].size() != 0) {
        //~ const char* dummy_args[] = { "dummy", "--argv",  options["FIM_application_argv"].c_str(), NULL };
        //~ int argc_dummy = sizeof(dummy_args)/sizeof(dummy_args[0]) - 1;
        //~ icmCLPP parser = icmCLParser("platform", ICM_AC_ALL);
        //~ icmCLParseArgs(parser, argc_dummy, dummy_args);
    //~ }

    ///Platfomr Variables
    processors      = (icmProcessorP*)          calloc ( NumberOfRunningFaults ,sizeof(icmProcessorP*));
    bus             = (icmBusP*)                calloc ( NumberOfRunningFaults ,sizeof(icmBusP*));
    pse             = (icmPseP*)                calloc ( NumberOfRunningFaults ,sizeof(icmPseP*));
    mem             = (icmMemoryP*)             calloc ( NumberOfRunningFaults ,sizeof(icmMemoryP*));
    pccb            = (icmWatchPointP*)         calloc ( NumberOfRunningFaults ,sizeof(icmWatchPointP*));
    lockup_intr     = (icmNetP*)                calloc ( NumberOfRunningFaults ,sizeof(icmNetP*));
    
    srand (time(NULL)*PlatformIdentifier);
    
///Platform*****************************************************************************************
    
    /// initialize OVPsim, enabling verbose mode to get statistics at end
    /// of execution
    icmInitPlatform(ICM_VERSION, (icmInitAttrs)icmAttrs, 0, 0,"FIM");
    
    ///Initilize the FIM platform
    initPlatform();
    
    if(IsBareMetalMode)
        BareMetalPlaform();
    else
        LinuxPlatform();
    
    if(IsFaultCampaignExecution) {
        loadFaults();
        printf("IsFaultCampaignExecution\n");
    }
    
///Simulation****************************************************************************************
        icmProcessorP stoppedProcessor;
        int StopReason,cpuid;
        bool SimulationFinished=false;
        
        while(!SimulationFinished){
            ///Simulate
            stoppedProcessor=icmSimulatePlatform();
            
            ///The processor that has caused simulation to stop, or null if no processor has caused simulation to stop (e.g. the stop time has been reached).
            if(stoppedProcessor==NULL)
                break;
            
            StopReason=icmGetStopReason(stoppedProcessor);
            
            icmPrintf("\n***simulation interrupted CODE %X CPU %s IC "FMT_64u"\n",StopReason,icmGetProcessorName(stoppedProcessor,"/"),icmGetProcessorICount(stoppedProcessor));
            sscanf(icmGetProcessorName(stoppedProcessor,"/"),"FIM/%d",&cpuid);
            
            //~ printf("SMP parent "FMT_64u"\n",icmGetProcessorICount(icmGetSMPParent(stoppedProcessor)));
            //~ printf("SMP PC     "FMT_64u"\n",icmGetPC(stoppedProcessor));
            //~ printf("SMP instruction %s  \n",icmDisassemble(stoppedProcessor,icmGetPC(stoppedProcessor)));
            //~ icmDumpTraceBuffer(stoppedProcessor);
            
            switch(StopReason) {
                case ICM_SR_FINISH:
                case ICM_SR_EXIT: {
                    SimulationFinished=true;
                    break; }
                case ICM_SR_BP_ICOUNT: {
                    if(IsGoldExecution)
                        GoldProfiling(cpuid);
                    else
                        FaultInjection(cpuid);
                    break; }
                case ICM_SR_WATCHPOINT: { 
                    handleWatchpoints(); 
                    break; }
                case ICM_SR_YIELD: { 
                    YieldAllCores(cpuid);
                    break; }
                case ICM_SR_INTERRUPT:{
                    YieldAllCores(cpuid);
                    break; }
                default: {
                    printf("Unexpected termination\n");
                    ///Update the PE status
                    updateStopReason(StopReason,cpuid);
                    ///Retrieve a processor
                    YieldAllCores(cpuid);
                }
            }
       }
       
       
///*Simulation End****************************************************************************************/
        printf("FIM: Report \n**********************************************************\n");
        report();
    
    icmTerminate();
    
printf("\n\n END ID: %d\n\n",PlatformIdentifier);
return 0;
}

