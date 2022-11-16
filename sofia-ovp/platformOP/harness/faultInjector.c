///System includes
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>

///Local includes from OVPSim
#include "op/op.h"

#include "../options.h"

///Fault Injector header
#include "faultInjector.h"

///Definition
//External memory to control the OVP-FIM -- not more in use
#define baseAddress 0xFA000000
#define len 0x0FF

//Main struct pointer
struct processorStruct* processorData;

//PE wrapper
#define PE processorData[processorNumber]

// Flags
Bool removeDumpFiles = True;

//Trace Flags
//~ Bool enablefunctionprofile  = True;
//~ Bool enablelinecoverage     = False;
Bool enableContexTrace      = False;

// Temporary string
#define STRING_SIZE 256
#define TMP_STRING_SIZE 1024

char tempString[TMP_STRING_SIZE];
char modelVariant[STRING_SIZE];

//File pointer
FILE* filePointer;

//Options
struct optionsS _options;

//Top level module
optModuleP topLevelModule;

/////////////////////////////////////////////////////////////////////////
// Macros and Defines
/////////////////////////////////////////////////////////////////////////
#include "../commonMacros.h"
/////////////////////////////////////////////////////////////////////////
// Structs and enumerators
/////////////////////////////////////////////////////////////////////////
#include "../commonStructsAndEnumerators.h"
/////////////////////////////////////////////////////////////////////////
// Functions
/////////////////////////////////////////////////////////////////////////
//
// update the stop reason
//
#define MACRO_UPDATE_STOP_REASON(_A) PE.stopReason=_A;
#define MACRO_FAULTTYPE(_A)          !strcmp(PE.faultTypeStr,faultTypes[_A])

int TraceReport ;

void register_bank_write( int address , int value )
{
    switch ( address ) {
        case 0x400022A0:
            opPrintf( "%c" , value );
            break;

        case 0x40002294:
            TraceReport = value;
            break;

        default: break;
    }
}

OP_BUS_SLAVE_WRITE_FN( regbank_wb )
{
    register_bank_write( addr , ( *( Uns32* )data ) );
}

//
// simulation management
//
void fimSimulate(struct optionsS _options, optModuleP _mi) {
    /// platform options
    options=_options;

    /// top level module
    topLevelModule = _mi;

    /// constructor
    initialize();

    /// platform identification
    opMessage("I",PREFIX_FIM,"Platform ID %u PID %d Variant %s\n",MACRO_PLATFORM_ID,getpid(),opProcessorVariant(processorData[0].processorObj));

    ///
    /// simulation flow
    ///
    optProcessorP stoppedProcessor;
    int stopReason,processorNumber=-2;
    Bool simulationFinished=False;

    while(!simulationFinished){
        //Simulator call

        opMessage("I",PREFIX_FIM,"Simulation starting (PROCESSOR) \n");
        stoppedProcessor = opRootModuleSimulate(topLevelModule);

        //If no processor has caused simulation to stop (e.g. the stop time has been reached).
        if(!stoppedProcessor) {
            opMessage("I",PREFIX_FIM,"Simulation stopped (MODULE) - %s SR %X \n", opStopReasonString(opRootModuleStopReason(topLevelModule)),opRootModuleStopReason(topLevelModule));
            break;
        }

        //The processor that has caused simulation to stop
        stopReason=opProcessorStopReason(stoppedProcessor);
        opMessage("I",PREFIX_FIM,"Simulation stopped (PROCESSOR) - %s SR %X CPU %s IC "FMT_64u"\n",opStopReasonString(stopReason),stopReason,opObjectName(stoppedProcessor),opProcessorICount(stoppedProcessor));
        sscanf(opObjectHierName(stoppedProcessor),"FIM/DUT%d",&processorNumber);

        switch(stopReason) {
            case OP_SR_FINISH:
            case OP_SR_EXIT: {
                simulationFinished=True;
                break; }
            case OP_SR_BP_ICOUNT: {
                if(MACRO_GOLD_PLATFORM_FLAG) {
                    handlerGoldProfiling(processorNumber);
                } else
                    handlerFaultInjection(processorNumber);
                break; }
            case OP_SR_WATCHPOINT: {
                //~ handleWatchpoints();
                break; }
            case OP_SR_YIELD: {
                removeFromScheduler(processorNumber);
                break; }
            case OP_SR_INTERRUPT:{ ///Intercept lib signal
                applicationRequestHandler(processorNumber);
                break; }
            default: {
                opMessage("I",PREFIX_FIM,"Unexpected termination\n");
                //Update the PE status
                MACRO_UPDATE_STOP_REASON(stopReason);
                //Retrieve a processor
                removeFromScheduler(processorNumber);
            }
        }
    }

    if(MACRO_GOLD_PLATFORM_FLAG)
        dumpGoldInformation();
    else
        createFaultInjectionReport();

    //Memory cleaning
    for (int processorNumber=0; processorNumber<MACRO_NUMBER_OF_FAULTS;processorNumber++) {
        free(PE.childrens);
        free(PE.expectedICount);
        free(PE.applicationBegan);
    }
    free(processorData);
}

//
// Constructor and options checker
//
void initialize() {
    /// allocate the array according with the number of DUTs
    processorData = (processorDataP) malloc (sizeof(processorDataS) * MACRO_NUMBER_OF_FAULTS);

    if(!strcmp(options.environment,"ovparmv7")) {
        numberOfRegistersToCompare=-2;
        do {numberOfRegistersToCompare++;}
        while(strcmp(END_LIST,possibleRegistersV7[numberOfRegistersToCompare]) != 0 );

        for(int i=0; i<numberOfRegistersToCompare; i++)
            strcpy(possibleRegisters[i],possibleRegistersV7[i]);
    } else if(!strcmp(options.environment,"ovparmv8")) {
        numberOfRegistersToCompare=-2;
        do {numberOfRegistersToCompare++;}
        while(strcmp(END_LIST,possibleRegistersV8[numberOfRegistersToCompare]) != 0 );

        for(int i=0; i<numberOfRegistersToCompare; i++)
            strcpy(possibleRegisters[i],possibleRegistersV8[i]);
    }
    else {
        //RISCV
        numberOfRegistersToCompare=-2;
        do {numberOfRegistersToCompare++;}
        while(strcmp(END_LIST,possibleRegistersRV[numberOfRegistersToCompare]) != 0 );

        for(int i=0; i<numberOfRegistersToCompare; i++)
            strcpy(possibleRegisters[i],possibleRegistersRV[i]);
    }

    /// create each processor handler
    for (int i=0; i<MACRO_NUMBER_OF_FAULTS;i++)
        initProcessorInstFIM(i);
}

//
// initialise ONE processor
//
void initProcessorInstFIM(Uns32 processorNumber) {
    //Define PE.faultIdentifier
    PE.faultIdentifier = processorNumber;

    //Get the module CA -- Review the modules naming
    sprintf(tempString,"DUT%d",processorNumber);
    optModuleP mod = opObjectByName (topLevelModule, tempString, OP_MODULE_EN).Module;

    //Get PE handlers for the processor and memory object
    PE.processorObj = opProcessorNext(mod, 0); // Get the first processor
    PE.memObj       = opMemoryNext(mod, 0);    // Get the first memory
    PE.busObj       = opBusNext(mod,0);        // Get the first bus
    PE.modObj       = mod;                     // Processor module
    PE.options      = options;                 // Platform options

    //Initialise the PE finished status
    PE.stopReason=FIM_SR_UNSET;

    //Calculate the number of instructions saved by the early termination
    PE.instructionsToNormalExit=0;

    //After kernel point
    PE.applicationBegan=0;

    //Allocate the arrays
    PE.childrens        = (optProcessorP*) calloc (MACRO_NUMBER_OF_CORES,sizeof(optProcessorP));
    PE.expectedICount   = (Uns64*)         calloc (MACRO_NUMBER_OF_CORES,sizeof(Uns64));
    PE.applicationBegan = (Uns64*)         calloc (MACRO_NUMBER_OF_CORES,sizeof(Uns64));
    PE.applicationEnd   = (Uns64*)         calloc (MACRO_NUMBER_OF_CORES,sizeof(Uns64));

    if(MACRO_BAREMETAL_MODE_FLAG) { // !!!! Only valid to one core processor
        if(!strcmp(options.environment,"ovparmv7") || !strcmp(options.environment,"riscv"))
            PE.childrens[0]=PE.processorObj;
        else //Using the multicore for baremetal armv8
            PE.childrens[0]=opProcessorChild(PE.processorObj);
    } else {
        if (MACRO_BIGLITTLE_FLAG) {
            // big Cores
            PE.childrens[0]=opProcessorChild(opProcessorChild(PE.processorObj));
            PE.childrens[1]=opProcessorSiblingNext(PE.childrens[0]);

            // little Cores
            PE.childrens[2]=opProcessorChild(opProcessorSiblingNext(opProcessorChild(PE.processorObj)));
            PE.childrens[3]=opProcessorSiblingNext(PE.childrens[2]);

            opPrintf("PATH %s\n",opObjectName(PE.childrens[0]));
            opPrintf("PATH %s\n",opObjectName(PE.childrens[1]));
            opPrintf("PATH %s\n",opObjectName(PE.childrens[2]));
            opPrintf("PATH %s\n",opObjectName(PE.childrens[3]));
        } else {
            //First core
            PE.childrens[0]=opProcessorChild(PE.processorObj);

            //Get all cores handlers
            for (int i=1;i<MACRO_NUMBER_OF_CORES;i++) {
                PE.childrens[i]= opProcessorSiblingNext(PE.childrens[i-1]);
            }
        }
    }

    //~ opBusSlaveNew( PE.busObj , "extMemReg" , 0 , OP_PRIV_RW , 0x40002290 , 0x400022B0 , NULL , regbank_wb , 0 , 0 );
    //Instantiate the FIM intercept and share a variable pointer as argument
    optParamP optAttr = opParamPtrSet(NULL   ,"processorData",(void*) &processorData[processorNumber]);
    opProcessorExtensionNew(PE.processorObj,options.interceptlib,"model",optAttr);

    //Create the external memory
    //~ sprintf(tempString,"FIM-MEM-%u",processorNumber);
    //~ opMemoryNew(mod,tempString,OP_PRIV_RWX,len,OP_CONNECTIONS(OP_BUS_CONNECTIONS(OP_BUS_CONNECT( PE.busObj , "sp1", .slave=1, .addrLo=baseAddress, .addrHi=baseAddress+len))),0);

    //Add the callback memory
    //~ opProcessorReadMonitorAdd(PE.processorObj, baseAddress, baseAddress+len, UI_faultInjector, (void*) ((intptr_t) PE.faultIdentifier));

    //Manually extract from the OVPFIM-ARMv7.elf disassemble code --> old
    //~ opProcessorApplicationSymbolAdd(PE.processorObj,"__ovp_init_external",    0x000105b4,4,ORD_SYMBOL_TYPE_FUNC,ORD_SYMBOL_TYPE_SECTION);
    //~ opProcessorApplicationSymbolAdd(PE.processorObj,"__ovp_segfault_external",0x000105ec,4,ORD_SYMBOL_TYPE_FUNC,ORD_SYMBOL_TYPE_SECTION);

    //Linux Overrides
    if(MACRO_LINUX_MODE_FLAG) {
        opParamStringOverride(mod,"smartLoader""/""kernel",options.zimage);
        opParamStringOverride(mod,"smartLoader""/""initrd",options.initrd);
        opParamUns32Override(mod, "smartLoader""/""bootaddr",0x60001000);//Required for the booatloader in multicores

        //Uart log file
        sprintf(tempString,"./PlatformLogs/uart0-%u",MACRO_PLATFORM_ID);
        opParamStringOverride(mod,"uart0""/""outfile",tempString);
        sprintf(tempString,"./PlatformLogs/uart1-%u",MACRO_PLATFORM_ID);
        opParamStringOverride(mod,"uart1""/""outfile",tempString);

        //Add extra files with symbols
        //opProcessorApplicationLoad(PE.processorObj,getenv("LINUX_VKERNEL"),OP_LDR_SYMBOLS_ONLY,0);
        //opProcessorApplicationLoad(PE.processorObj,options.zimage,OP_LDR_SYMBOLS_ONLY,0);
        //opProcessorApplicationLoad(PE.processorObj,getenv("LINUX_BOOTLOADER"),OP_LDR_SYMBOLS_ONLY,0);
        //opProcessorApplicationLoad(PE.processorObj,getenv("OVPFIM_EXEC"),OP_LDR_SYMBOLS_ONLY,0);

        if(MACRO_LINUX_CONSOLE_FLAG) {
            opParamBoolOverride(mod,"uart0""/""console",1);
            opParamBoolOverride(mod,"uart1""/""console",0);
        }

        if(!MACRO_LINUX_GRAPHICS_FLAG)
            opParamBoolOverride(mod,"lcd1""/""noGraphics",1);
    }else if (MACRO_LINUX64_MODE_FLAG){
        opPrintf("MACRO_LINUX64_MODE_FLAG\n");
        // Linux AARCH64

        if (MACRO_BIGLITTLE_FLAG) {
            opParamStringOverride(mod,"smartLoader""/""disable","T");
        } else {
            opParamStringOverride(mod,"smartLoader""/""kernel",options.zimage);
            opParamStringOverride(mod,"smartLoader""/""initrd",options.initrd);
            //~ opParamUns32Override(mod, "smartLoader""/""bootaddr",0x60001000);
            opParamStringOverride(mod,"smartLoader""/""dtb",options.linuxdtb);
        }

        //Uart log file
        sprintf(tempString,"./PlatformLogs/uart0-%u",MACRO_PLATFORM_ID);
        opParamStringOverride(mod,"uart0""/""outfile",tempString);
        sprintf(tempString,"./PlatformLogs/uart1-%u",MACRO_PLATFORM_ID);
        opParamStringOverride(mod,"uart1""/""outfile",tempString);

        //Add extra files with symbols
        //opProcessorApplicationLoad(PE.processorObj,getenv("LINUX_VKERNEL"),OP_LDR_SYMBOLS_ONLY,0);
        //opProcessorApplicationLoad(PE.processorObj,options.zimage,OP_LDR_SYMBOLS_ONLY,0);
        //opProcessorApplicationLoad(PE.processorObj,getenv("OVPFIM_EXEC"),OP_LDR_SYMBOLS_ONLY,0);

        if(MACRO_LINUX_CONSOLE_FLAG) {
            opParamBoolOverride(mod,"uart0""/""console",1);
            opParamBoolOverride(mod,"uart1""/""console",0);
        }

    } else {
        //Load processor memory
        //~ opProcessorApplicationLoad(PE.processorObj,options.FIM-application, OP_LDR_SET_START , 0);
    }

    ///Get information about the gold execution
    if(MACRO_FI_PLATFORM_FLAG) {
        for (int i=0;i<MACRO_NUMBER_OF_CORES;i++) {
            /// reads the gold count for each core
            sprintf(tempString,"echo $(grep \"Executed Instructions Per core=\" %s/"FILE_GOLD_INFORMATION" | cut -d '=' -f2 | cut -d ',' -f%d)",MACRO_APPLICATION_FOLDER,i+1);
            filePointer = popen(tempString, "r");
            if( fgets(tempString,STRING_SIZE,filePointer) == NULL ) opMessage("I",PREFIX_FIM,"Command execution error\n");
            sscanf(tempString,""FMT_64u"",&PE.expectedICount[i]);
            pclose(filePointer);

            /// reads the application began for each core
            sprintf(tempString,"echo $(grep \"Application begins=\" %s/"FILE_GOLD_INFORMATION" | cut -d '=' -f2 | cut -d ',' -f%d)",MACRO_APPLICATION_FOLDER,i+1);
            filePointer = popen(tempString, "r");
            if( fgets(tempString,STRING_SIZE,filePointer) == NULL ) opMessage("I",PREFIX_FIM,"Command execution error\n");
            sscanf(tempString,""FMT_64u"",&PE.applicationBegan[i]);
            pclose(filePointer);
        }
    }
}

//
// Load the gold profiler
//
void setProfiling() { // !!!! Only valid to one core processor
    Uns32 processorNumber = 0;
    ///Used to configure the interval between checkpoints
    PE.expectedICount[0]=MACRO_CHECKPOINT_INTERVAL;
    ///Set the first checkpoint
    opProcessorBreakpointICountSet(PE.processorObj,MACRO_CHECKPOINT_INTERVAL);
    ///State
    PE.status=GOLD_PROFILE;

    ///Create the checkpoint ZERO
    opProcessorStateSaveFile(PE.processorObj,FOLDER_CHECKPOINTS"/"FILE_NAME_REG_CHECKPOINT"-0");
    opMemoryStateSaveFile(PE.memObj,         FOLDER_CHECKPOINTS"/"FILE_NAME_MEM_CHECKPOINT"-0");
    opMemoryStateRestoreFile(PE.memObj,      FOLDER_CHECKPOINTS"/"FILE_NAME_MEM_CHECKPOINT"-0");
    opMemoryStateSaveFile(PE.memObj,         FOLDER_CHECKPOINTS"/"FILE_NAME_MEM_CHECKPOINT"-0");
}

//
// Load the faults
//
void loadFaults() {
    char checkpointFileName[STRING_SIZE];

    /// open fault list
    sprintf(tempString,"%s/"FILE_FAULT_LIST,MACRO_APPLICATION_FOLDER);
    filePointer  = fopen (tempString,"r");

    if (filePointer == NULL) {
        opMessage("F",PREFIX_FIM,"Fault list file can't be used \n ");
    }

    /// skip header file
    if( fgets(tempString,STRING_SIZE,filePointer) == NULL )     opMessage("I",PREFIX_FIM,"Command execution error\n"); // @Error

    /// getting the right file position
    for(int i=0;i < (MACRO_PLATFORM_ID-1)*MACRO_NUMBER_OF_FAULTS ;i++)
        if( fgets(tempString,STRING_SIZE,filePointer) == NULL ) opMessage("I",PREFIX_FIM,"Command execution error\n"); // @Error

    int core = 0;
    /// load the faults
    for(int processorNumber=0;processorNumber<MACRO_NUMBER_OF_FAULTS;processorNumber++) {
        /// get one line
        if( fgets(tempString,STRING_SIZE,filePointer) == NULL ) opMessage("I",PREFIX_FIM,"Command execution error\n"); // @Error

        /// extract the info
        sscanf(tempString,"%d %s %s %d "FMT_64u":%d "FMT_64x" \n",&PE.faultIdentifier,PE.faultTypeStr,PE.faultTarget,&PE.faultRegisterPosition,&PE.faultInsertionTime,&core,&PE.faultValue);

        /// core to inject
        PE.coreToInject=PE.childrens[core];
        PE.faultDuration = 1; // @Review

        /// register fault based on instruction count
        if(MACRO_FAULTTYPE(TYPE_REGISTER) || MACRO_FAULTTYPE(TYPE_FUNCTIONTRACE) || MACRO_FAULTTYPE(TYPE_FUNCTIONTRACE2)) {
            strcpy(PE.faultRegister,possibleRegisters[PE.faultRegisterPosition]);

            /// adjusting the fault to use checkpoint
            if(MACRO_CHECKPOINT_ENABLE_FLAG) { // !!!! Only valid to one core processor
                PE.checkpoint=PE.faultInsertionTime/MACRO_CHECKPOINT_INTERVAL;

                sprintf(checkpointFileName,FOLDER_CHECKPOINTS"/"FILE_NAME_REG_CHECKPOINT"-"FMT_64u,PE.checkpoint);
                opProcessorStateRestoreFile(PE.processorObj,checkpointFileName);

                sprintf(checkpointFileName,FOLDER_CHECKPOINTS"/"FILE_NAME_MEM_CHECKPOINT"-"FMT_64u,PE.checkpoint);
                opMemoryStateRestoreFile(PE.memObj,checkpointFileName);

                /// adjust Fault the absolute position to a relative position
                PE.faultInsertionTime=PE.faultInsertionTime - (PE.checkpoint*MACRO_CHECKPOINT_INTERVAL);

                opMessage("I",PREFIX_FIM,"Relative position "FMT_64u" # Checkpoint "FMT_64u"\n",PE.faultInsertionTime,PE.checkpoint);
                PE.checkpointCounter=0;
            }

            /// set breakpoint
            opProcessorBreakpointICountSet(PE.coreToInject,PE.faultInsertionTime);
            opMessage("I",PREFIX_FIM,"Loaded Fault %d "FMT_64u" 0x"FMT_64x" %d (%s) core %d\n",PE.faultIdentifier,PE.faultInsertionTime,PE.faultValue,PE.faultRegisterPosition,PE.faultRegister,core); // @Info

        } else  if(MACRO_FAULTTYPE(TYPE_VARIABLE)) {
            opMessage("I",PREFIX_FIM,"Loaded Fault %d "FMT_64u" 0x"FMT_64x" %d TYPE %s\n",PE.faultIdentifier,PE.faultInsertionTime,PE.faultValue,PE.faultRegisterPosition,PE.faultTypeStr); // @Info

        } else  if( MACRO_FAULTTYPE(TYPE_MEMORY) || MACRO_FAULTTYPE(TYPE_VARIABLE2) || MACRO_FAULTTYPE(TYPE_MEMORY2) || MACRO_FAULTTYPE(TYPE_FUNCTIONCODE)) {
            // @Info the faultTarget is a 16 hex number
            opMessage("I",PREFIX_FIM,"Loaded Fault %d "FMT_64u" 0x"FMT_64x" %d core %d address %s \n",PE.faultIdentifier,PE.faultInsertionTime,PE.faultValue,PE.faultRegisterPosition,core,PE.faultTarget); // @Info
            opProcessorBreakpointICountSet(PE.coreToInject,PE.faultInsertionTime);
        } else  {
            // @Error - type not recognised
            opMessage("F",PREFIX_FIM,"Loaded Fault %d not recognized: %s\n",PE.faultIdentifier,PE.faultTypeStr);
        }

        /// PE status
        PE.status=BEFORE_INJECTION;
    }
    fclose(filePointer);
}

//
// initialise using already created cpu components
//
static OP_PRE_SIMULATE_FN(modulePreSimulate) {
    opMessage("I",PREFIX_FIM,"Presimulation \n");

    if(MACRO_FI_PLATFORM_FLAG)
        loadFaults();

    if(MACRO_CHECKPOINT_ENABLE_FLAG && MACRO_GOLD_PLATFORM_FLAG)
        setProfiling();
}

static OP_SIMULATE_STARTING_FN(moduleSimulateStart) {
// insert simulation starting code here

}

//
// Release all cores in multi core or single core cpu
//
void removeFromScheduler(Uns32 processorNumber) {
    for (int i=0;i<MACRO_NUMBER_OF_CORES;i++) {
        opProcessorFreeze(PE.childrens[i]); // retrieves from the simulator scheduler
    }

    // freeze the parent also
    opProcessorFreeze(PE.processorObj);
}

//
// handle the application requests using the symbol intercept library
//
void applicationRequestHandler(int processorNumber) {
    switch(PE.stopReason) {
        case FIM_SR_SERVICE_BEGIN: {     /// Application starting point
            opMessage("I",PREFIX_FIM,"FIM_SR_SERVICE_BEGIN\n");

            //Save the application entry point
            for (int i=0;i<MACRO_NUMBER_OF_CORES;i++)
                PE.applicationBegan[i]=opProcessorICount(PE.childrens[i]);

            //Reset the stop reason
            MACRO_UPDATE_STOP_REASON(FIM_SR_UNSET)

            // Setup command
            /*
            const char* cmdToExecuteLog = "cpu_CPU1/vapHelper/logtofile";
            optCommandP cmdLog = opObjectByName(PE.modObj,cmdToExecuteLog,OP_COMMAND_EN).Command;

            // Setup calling arguments
            const char *cmdLogArgv[] = {"logtofile","-on","-filename","contexttrace.log"};
            opCommandCall(cmdLog,4,cmdLogArgv); */

            if(enableContexTrace && MACRO_GOLD_PLATFORM_FLAG) { /// dump all function calls and returns
                /// setup command
                const char* cmdToExecute = "cpu_CPU0/vapHelper/contexttrace";
                optCommandP cmd = opObjectByName(PE.modObj,cmdToExecute,OP_COMMAND_EN).Command;

                /// setup calling arguments
                const char *cmdArgv[] = {"contexttrace", "-on"};
                opCommandCall(cmd,2,cmdArgv);
            }

            if(PE.options.enablefunctionprofile && MACRO_GOLD_PLATFORM_FLAG) { /// create a function profile
                /// setup command
                const char* cmdToExecute = "cpu_CPU0/vapTools/functionprofile"; //attaching the cpu 0 but should work for all cores!
                optCommandP cmd = opObjectByName(PE.modObj,cmdToExecute,OP_COMMAND_EN).Command;

                /// setup calling arguments
                const char *cmdArgv[] = {"functionprofile", "-dotfile", "app.dot" ,"-sampleinterval","1"};
                opCommandCall(cmd,5,cmdArgv);
            }

            if(PE.options.enablelinecoverage && MACRO_GOLD_PLATFORM_FLAG) { /// create the line coverage graph
                Uns32 processorNumber = 0;

                /// setup command
                const char* cmdToExecute = "cpu_CPU0/vapTools/linecoverage";
                optCommandP cmd = opObjectByName(PE.modObj,cmdToExecute,OP_COMMAND_EN).Command;

                /// setup calling arguments
                const char *cmdArgv[] = {"linecoverage", "-on", "-filename" ,"ArmVersatileExpress_cpu_CPU0.lcov"};
                opCommandCall(cmd,4,cmdArgv);
            }

            if(PE.options.enableitrace && MACRO_GOLD_PLATFORM_FLAG) { /// Display intruction trace
                Uns32 processorNumber = 0;

                /// setup command
                const char* cmdToExecute = "cpu/itrace";
                optCommandP cmd = opObjectByName(PE.modObj,cmdToExecute,OP_COMMAND_EN).Command;
                opCommandCall(cmd,0,0);
            }

            if(PE.options.enableftrace) { /// create the line coverage graph
				Uns32 processorNumber = 0;

				/// setup command
				const char* cmdToExecute = "cpu_CPU0/cpuEstimator/instTrace";
				optCommandP cmd = opObjectByName(PE.modObj,cmdToExecute,OP_COMMAND_EN).Command;
				char* filename = malloc(sizeof(char) * 50);

				if (MACRO_GOLD_PLATFORM_FLAG){
					sprintf(filename,"Profiling/profiling-gold");
							const char *cmdArgv[] = {"instTrace","-active", "-filename", filename};
							opCommandCall(cmd,4,cmdArgv);
				}
				else{
						sprintf(filename, "Profiling/profiling-%d",PE.faultIdentifier);
					char fiTime[1024];
					sprintf(fiTime, FMT_64u, PE.faultInsertionTime);
							const char *cmdArgv[] = {"instTrace","-active", "-filename", filename, "-fault", fiTime, "-targetReg", PE.faultRegister};
							opCommandCall(cmd,8,cmdArgv);
				}
                /// setup calling arguments
				free(filename);
          }

            break;}
        case FIM_SR_SERVICE_EXIT: {      /// Application finishes calling the exit()
            opMessage("I",PREFIX_FIM,"FIM_SR_SERVICE_EXIT\n");
            for (int i=0;i<MACRO_NUMBER_OF_CORES;i++)
                PE.applicationEnd[i]=opProcessorICount(PE.childrens[i]);
            MACRO_UPDATE_STOP_REASON(FIM_SR_UNSET)
            removeFromScheduler(processorNumber);
            break;}
        case FIM_SR_SERVICE_SEG_FAULT: { /// Segmentation fault detected
            opMessage("I",PREFIX_FIM,"FIM_SR_SERVICE_SEG_FAULT\n");
            for (int i=0;i<MACRO_NUMBER_OF_CORES;i++)
                PE.applicationBegan[i]=opProcessorICount(PE.childrens[i]);
            MACRO_UPDATE_STOP_REASON(FIM_SR_SEG_FAULT)
            removeFromScheduler(processorNumber);
            break;}
        default: {
            removeFromScheduler(processorNumber);
            break;}
        }
}

//
// Extract the information about the faultless execution
//
void dumpGoldInformation() {
    /// only one gold count --> used in the PE macro
    Uns32 processorNumber = 0;

    /// file name
    char goldName[STRING_SIZE];

    /// save the final PE memory state
    sprintf(goldName,"%s/%s/%s-%d",MACRO_APPLICATION_FOLDER,FOLDER_DUMPS,FILE_NAME_MEM_GOLD,0);
    opMemoryFlush(PE.memObj,0x0,0xFFFFFFFF);
    opMemoryStateSaveFile(PE.memObj,goldName);

    /// Save the final PE internal state
    sprintf(goldName,"%s/%s/%s-%d",MACRO_APPLICATION_FOLDER,FOLDER_DUMPS,FILE_NAME_REG_GOLD,0);
    opProcessorStateSaveFile(PE.processorObj,goldName);

    /// remove the header file --only realy need for multicore cpus
    sprintf(tempString,"A=$(grep -m%d END_CORE %s -n | tail -n1 | cut -d\":\" -f1); sed -i \"1,$(echo $A)d\" %s",MACRO_NUMBER_OF_CORES,goldName,goldName);
    if( system(tempString) ) opMessage("I",PREFIX_FIM,"Command execution error (%s)\n",tempString);

    /// create file
    sprintf(tempString,"%s/"FILE_GOLD_INFORMATION,MACRO_APPLICATION_FOLDER);
    filePointer = fopen (tempString,"w");

    /// save the number of executed instruction (main object or parent)
    fprintf(filePointer,"Application # of instructions="FMT_64u"\n",opProcessorICount(PE.processorObj));


    if(MACRO_BAREMETAL_MODE_FLAG) {
        fprintf(filePointer,"Executed Instructions Per core="FMT_64u"\n",opProcessorICount(PE.childrens[0]));
        fprintf(filePointer,"Application begins="FMT_64u"\n",PE.applicationBegan[0]);

        /// Application instruction after kernel boot discount
        fprintf(filePointer,"Application # without kernel="FMT_64u"\n",(Uns64) opProcessorICount(PE.childrens[0]) - PE.applicationBegan[0] );
    } else {
        Uns64 totalBegin=0,totalEnd=0;
        ///
        fprintf(filePointer,"Executed Instructions Per core="FMT_64u"",opProcessorICount(PE.childrens[0]));
        totalEnd+=opProcessorICount(PE.childrens[0]);
        for (int i=1;i<MACRO_NUMBER_OF_CORES;i++){
            fprintf(filePointer,","FMT_64u"",opProcessorICount(PE.childrens[i]));
            totalEnd+=opProcessorICount(PE.childrens[i]);
            };
        fprintf(filePointer,"\n");
        ///
        fprintf(filePointer,"Application begins="FMT_64u"",PE.applicationBegan[0]);
        totalBegin+=PE.applicationBegan[0];
        for (int i=1;i<MACRO_NUMBER_OF_CORES;i++) {
            fprintf(filePointer,","FMT_64u"",PE.applicationBegan[i]);
            totalBegin+=PE.applicationBegan[i];
            }
        fprintf(filePointer,"\n");

        /// Application instruction after kernel boot discount
        printf("Application # without kernel="FMT_64u"\n",(Uns64) totalBegin);
        printf("Application # without kernel="FMT_64u"\n",(Uns64) totalEnd );
        fprintf(filePointer,"Application # without kernel="FMT_64u"\n",(Uns64) totalEnd - totalBegin);

    }

    Uns64 tempRegister;
    /// extract the register context
    fprintf(filePointer,"Integer Registers=");
    for (int i=0;i<MACRO_NUMBER_OF_CORES;i++)
        for (int j=0;j<numberOfRegistersToCompare;j++) {
            opProcessorRegReadByName(PE.childrens[i],possibleRegisters[j],&tempRegister);
            fprintf(filePointer," "FMT_64u,tempRegister);
        }
    fprintf(filePointer,"\n");

    /// gem5 compatibility
    fprintf(filePointer,"Kernel Boot # of instructions=0\n");
    fprintf(filePointer,"Number of Ticks=0\n");
    fclose (filePointer);
}

//
// Memory callback to handler the application requests
//
OP_MONITOR_FN(UI_faultInjector) {
    /// inform the id of the processor wich triggered the callback
    int processorNumber = (intptr_t) userData;
    /// fet the service
    unsigned int code = (unsigned int) addr - baseAddress;
    //~ opMessage("I",PREFIX_FIM,"faultInjector IC "FMT_64u"\n",opProcessorICount(processor));

    if (code == FIM_SERVICE_BEGIN_APP) {
        opMessage("I",PREFIX_FIM,"Application began: ");
        //Get all counter
        for (int i=0;i<MACRO_NUMBER_OF_CORES;i++) {
            PE.applicationBegan[i]=opProcessorICount(PE.childrens[i]);
            opPrintf(""FMT_64u" ",PE.applicationBegan[i]);
            }
        opPrintf("\n");
    } else
    if (code == FIM_SERVICE_END_APP) {
        opMessage("I",PREFIX_FIM,"Application End: ");
        /// Get all counters
        for (int i=0;i<MACRO_NUMBER_OF_CORES;i++) {
            opPrintf(""FMT_64u" ",opProcessorICount(PE.childrens[i]));
            }
        opPrintf("\n");
        opProcessorYield(processor);
    } else
    if (code == FIM_SERVICE_SEG_FAULT) {
        opMessage("I",PREFIX_FIM,"Segmentation fault\n");
        MACRO_UPDATE_STOP_REASON(FIM_SR_SEG_FAULT)
        opProcessorYield(processor);
    } else {
        opMessage("I",PREFIX_FIM,"Unidentified cause \n");
        MACRO_UPDATE_STOP_REASON(FIM_SR_UNIDENTIFIED)
        opProcessorYield(processor);
    }
}

//
// Extract and compare the register context (Need improvements - acquire the gold registers before)
// return: 0 equal 1 different
int compareRegisterState(Uns32 processorNumber) {
Uns32 tempRegister,goldRegister=0; //32 bits registers
int   regCmp = 0;

for (int i=0;i<MACRO_NUMBER_OF_CORES;i++)
    for (int j=0;j<numberOfRegistersToCompare;j++) {
        opProcessorRegReadByName(PE.childrens[i],possibleRegisters[j],&tempRegister);

        sprintf(tempString,"grep \"Integer Registers=\" "FILE_GOLD_INFORMATION" | cut -d\' \' -f%d",j+3+numberOfRegistersToCompare*i);
        filePointer = popen(tempString, "r");
        if(!fgets(tempString,STRING_SIZE,filePointer)) //Get the command output
            goldRegister= strtoul(tempString,NULL,10);

        if(goldRegister!=tempRegister){
            opMessage("I",PREFIX_FIM," Mismatch Reg %3s %u %u \n",possibleRegisters[j],goldRegister,tempRegister);
            regCmp = 1;
        }
    }
fprintf(filePointer,"\n");
return regCmp;
}

//
// Analyses the fault injection and create a report
//
void createFaultInjectionReport() {
    char self_faultIndex                [STRING_SIZE];
    char self_faultType                 [STRING_SIZE];
    char self_faultRegister             [STRING_SIZE];
    char self_faultRegisterIndex        [STRING_SIZE];
    char self_faultTime                 [STRING_SIZE];
    char self_faultMask                 [STRING_SIZE];
    char self_faultAnalysis_name        [STRING_SIZE];
    char self_faultAnalysis_value       [STRING_SIZE];
    char self_ExecutedTicks             [STRING_SIZE];
    char self_ExecutedInstructions      [STRING_SIZE];
    char Self_MemInconsistency          [STRING_SIZE];
    char self_correctCheckpoint         [STRING_SIZE];
    char self_TraceReport               [STRING_SIZE];

    int code = FIM_SR_UNIDENTIFIED;
    int memCmp,regCmp,processorNumber;
    Uns64 count,goldCount;
    Uns64 totalExecInst=0;
    Uns64 checkpointLoaded = 0;

    char dumpName[STRING_SIZE];
    char goldName[STRING_SIZE];
    char reportFile[STRING_SIZE];
    char cmpReturn[STRING_SIZE];
    FILE * filePointer;

 /********************************************/
    sprintf(reportFile,"%s/"FOLDER_REPORTS"/reportfile-%d",MACRO_APPLICATION_FOLDER,MACRO_PLATFORM_ID);
    FILE * pFile = fopen (reportFile,"w");

    /// create file header
    fprintf(pFile,"%8s %14s %10s %4s %18s %18s %28s %7s %25s %22s %18s %15s %15s\n",
            "Index","Type","Target","i","Insertion Time","Mask","Fault Injection Result",
            "Code", "Execution Time (Ticks)", "Executed Instructions", "Mem Inconsistency",
            "Checkpoint", "Trace Variable");

 /********************************************/
    /// PE (DUT) processing
    for (processorNumber=0; processorNumber<MACRO_NUMBER_OF_FAULTS; processorNumber++) {
        strcpy(Self_MemInconsistency,"Null");

        totalExecInst = 0; /// number of executed instructions in all cores
        count         = 0;
        goldCount     = 0;

        /// acquire the gold count and the actual instruction count
        for (int i=0;i<MACRO_NUMBER_OF_CORES;i++) {
            count       += opProcessorICount(PE.childrens[i]);
            goldCount   += PE.expectedICount[i];
        }

        /// number of simulated instructions
        if(MACRO_CHECKPOINT_ENABLE_FLAG) { // !!!! Only valid to one core processor
            totalExecInst    += count - ( PE.checkpoint * (Uns64) MACRO_CHECKPOINT_INTERVAL);
            checkpointLoaded  = PE.checkpoint*MACRO_CHECKPOINT_INTERVAL;
        } else {
            totalExecInst+=count;
        }

        /// get PE stop reason
        int stopReason = opProcessorStopReason(PE.processorObj);

        /// no exception was detected during the simulation
        if (PE.stopReason == FIM_SR_UNSET) {
            PE.stopReason=stopReason;
            TraceReport = 8;
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            //                                                  Test Memory                                                               //
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            /// memory comparison
            opMemoryFlush(PE.memObj,0x0,0xFFFFFFFFF);/// flush the memory to avoid OVP dump format glitch

            /// memory dump files
            sprintf(dumpName,"%s/%s/%s-%d-%d",MACRO_APPLICATION_FOLDER,FOLDER_DUMPS,FILE_NAME_MEM_FAULT,processorNumber,MACRO_PLATFORM_ID);
            sprintf(goldName,"%s/%s/%s-%d",   MACRO_APPLICATION_FOLDER,FOLDER_DUMPS,FILE_NAME_MEM_GOLD,0);

            /// dump File
            opMemoryStateSaveFile(PE.memObj,dumpName);

            /// compare two text files
            strcpy(cmpReturn,"Null");
            sprintf(tempString,"cmp %s %s ",dumpName,goldName);
            opPrintf("%s\n",tempString);
            filePointer = popen(tempString, "r");
            if(fgets(cmpReturn,STRING_SIZE,filePointer)) opMessage("I",PREFIX_FIM,"Command execution error\n");
            pclose(filePointer);

            /// copy the correct string with the result of the memory comparison
            if(strcmp(cmpReturn,"Null")==0) {
                memCmp = 0;
                strcpy(Self_MemInconsistency,"No");
            } else {
                memCmp = 1;
                strcpy(Self_MemInconsistency,"Yes");
            }

            /// Retest the memory in case of memory fault injection
            if (MACRO_FAULTTYPE(TYPE_MEMORY) || MACRO_FAULTTYPE(TYPE_FUNCTIONCODE) || MACRO_FAULTTYPE(TYPE_MEMORY2) ){ // Memory faults
                // Undo the fault
                Uns8  memValue;
                Uns64 memAddress;

                // Acquire the address
                sscanf(PE.faultTarget,"%lX",&memAddress);

                if ( MACRO_FAULTTYPE(TYPE_MEMORY) ) { //Physical Memory
                    // Read the Physical Memory
                    opMemoryRead(PE.memObj,memAddress,&memValue,1,1);
                    // Apply the mask
                    memValue= ~(memValue ^ (Uns8) PE.faultValue);
                    // Write the Physical Memory
                    opMemoryWrite(PE.memObj,memAddress,&memValue,1,1);
                    // Flush memory
                    opMemoryFlush(PE.memObj,memAddress,memAddress);
                } else { // VA memory
                    // Read the VA
                    opProcessorRead(PE.childrens[0],memAddress, &memValue, 1, 1, True, OP_HOSTENDIAN_TARGET);
                    // Apply the mask
                    memValue= ~(memValue ^ (Uns8) PE.faultValue);
                    // Write the VA
                    opProcessorWrite(PE.childrens[0],memAddress, &memValue, 1, 1, True, OP_HOSTENDIAN_TARGET);
                    // Flush memory
                    opProcessorFlush(PE.childrens[0],memAddress,memAddress);
                }

                // dump File
                opMemoryStateSaveFile(PE.memObj,dumpName);

                // compare two text files
                strcpy(cmpReturn,"Null");
                sprintf(tempString,"cmp %s %s ",dumpName,goldName);
                filePointer = popen(tempString, "r");
                if(fgets(cmpReturn,STRING_SIZE,filePointer)) opMessage("I",PREFIX_FIM,"Command execution error 2\n");
                pclose(filePointer);

                // Check if memory is equal to the gold after the fault undo
                if(!strcmp(cmpReturn,"Null") && memCmp) // Mem. is dirty only
                    {code = SDC2; opPrintf("SDC2\n");}
            }

            /// remove dump file to reduce the HD footprint
            if(removeDumpFiles) {
                sprintf(tempString,"rm %s",dumpName);
                if( system(tempString) ) opMessage("I",PREFIX_FIM,"Command execution error (%s)\n",tempString);
            }

            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            //                                                  Test Reg Context                                                          //
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            /// register final state comparison
            sprintf(dumpName,"%s/%s/%s-%d-%d",MACRO_APPLICATION_FOLDER,FOLDER_DUMPS,FILE_NAME_REG_FAULT,processorNumber,MACRO_PLATFORM_ID);
            sprintf(goldName,"%s/%s/%s-%d",   MACRO_APPLICATION_FOLDER,FOLDER_DUMPS,FILE_NAME_REG_GOLD,0);

            /// dump File
            opProcessorStateSaveFile(PE.processorObj,dumpName);
            /// cut the header
            // Remove the header file --only need for multicore cpus
            sprintf(tempString,"A=$(grep -m%d END_CORE %s -n | tail -n1 |cut -d\":\" -f1); sed -i \"1,$(echo $A)d\" %s",MACRO_NUMBER_OF_CORES,dumpName,dumpName);
            if(system(tempString)) opMessage("I",PREFIX_FIM,"Command execution error (%s)\n",tempString);

            /// compare two text files
            strcpy(cmpReturn,"Null");
            sprintf(tempString,"cmp %s %s ",dumpName,goldName);
            printf("%s\n",tempString);
            filePointer = popen(tempString, "r");
            if(fgets(cmpReturn,STRING_SIZE,filePointer)) opMessage("I",PREFIX_FIM,"Command execution error\n");
            pclose(filePointer);

            /// remove dump file to reduce the HD footprint
            //~ if(removeDumpFiles) {
                //~ sprintf(tempString,"rm %s",dumpName);
                //~ if( system(tempString) ) opMessage("I",PREFIX_FIM,"Command execution error (%s)\n",tempString);
            //~ }

            if(strcmp(cmpReturn,"Null")==0)
                regCmp = 0;
            else
                regCmp = 1;

            /// register final state comparison
            //~ regCmp=compareRegisterState(processorNumber);

            /// Define the classification
            if(count != goldCount) { //Control flow error
                if(!memCmp){    code=Control_Flow_Data_OK;
                } else {        code=Control_Flow_Data_ERROR; }
            /// Internal State
            }else if(regCmp) { // Reg. Context
                if(!memCmp) {   code=REG_STATE_Data_OK;
                } else {        code=REG_STATE_Data_ERROR; }
            /// SDC
            }else if(memCmp) { //In case of SDC2; don't reassign
                if(code != SDC2) { code=SDC; }
            /// Masked fault
            }else {             code=Masked_Fault; TraceReport = 0; }

        /// an exception was detected during the simulation
        }else if (PE.stopReason == OP_SR_RD_PRIV)       { code = RD_PRIV;
        }else if (PE.stopReason == OP_SR_WR_PRIV)       { code = WR_PRIV;
        }else if (PE.stopReason == OP_SR_RD_ALIGN)      { code = RD_ALIGN;
        }else if (PE.stopReason == OP_SR_WR_ALIGN)      { code = WR_ALIGN;
        }else if (PE.stopReason == OP_SR_FE_PRIV)       { code = FE_PRIV;
        }else if (PE.stopReason == OP_SR_FE_ABORT)      { code = FE_ABORT;
        }else if (PE.stopReason == OP_SR_ARITH)         { code = ARITH;
        }else if (PE.stopReason == FIM_SR_Hard_Fault)   { code = Hard_Fault;
        }else if (PE.stopReason == FIM_SR_HANG)         { code = Hang;
        }else if (PE.stopReason == FIM_SR_LOCKUP)       { code = Lockup;
        }else if (PE.stopReason == FIM_SR_SEG_FAULT)    { code = SEG_FAULT;
        }else if (PE.stopReason == FIM_SR_SDC2)         { code = SDC2;
        }else if (PE.stopReason == FIM_SR_SDC3)         { code = SDC3;
        }else if (PE.stopReason == FIM_SR_MASKED)       { code = Masked_Fault;
        }else                                           { code = Unknown;}

        switch ( TraceReport ) {
            case 0:
            sprintf(self_TraceReport, "CORRECT");
            break;
            default:
            sprintf(self_TraceReport,   "ERROR");
            break;
        }

        ///Prepare to write in the file
        sprintf(self_faultIndex,            "%d",PE.faultIdentifier);
        sprintf(self_faultType,             "%s",PE.faultTypeStr);

        if(MACRO_FAULTTYPE(TYPE_REGISTER) || MACRO_FAULTTYPE(TYPE_FUNCTIONTRACE) || MACRO_FAULTTYPE(TYPE_FUNCTIONTRACE2))
            sprintf(self_faultRegister,     "%s",PE.faultRegister);
        else
            sprintf(self_faultRegister,     "%s",PE.faultTarget);

        sprintf(self_faultRegisterIndex,    "%u",PE.faultRegisterPosition);
        sprintf(self_faultTime,             ""FMT_64u"",PE.faultInsertionTime);
        sprintf(self_faultMask,             ""FMT_64x"",PE.faultValue);
        sprintf(self_faultAnalysis_name,    "%s",errorNames[code]);
        sprintf(self_faultAnalysis_value,   "%d",code);
        sprintf(self_ExecutedTicks,"0");                                    //Always zero in the OVP
        sprintf(self_ExecutedInstructions,  ""FMT_64u"",totalExecInst);
        //sprintf(self_MemInconsistency,"%s");
        sprintf(self_correctCheckpoint,     ""FMT_64u"",checkpointLoaded+PE.instructionsToNormalExit);

        fprintf(pFile,"%8s %14s %10s %4s %18s %18s %28s %7s %25s %22s %18s %15s %15s\n",
                    self_faultIndex,
                    self_faultType,
                    self_faultRegister,
                    self_faultRegisterIndex,
                    self_faultTime,
                    self_faultMask,
                    self_faultAnalysis_name,
                    self_faultAnalysis_value,
                    self_ExecutedTicks,
                    self_ExecutedInstructions,
                    Self_MemInconsistency,
                    self_correctCheckpoint,
                    self_TraceReport);
    }
    fclose (pFile);
}

//
// Profile the gold execution and create the checkpoints
//
// OBS: Only valid to one core processor
void handlerGoldProfiling(Uns32 processorNumber) {
    opMessage("I",PREFIX_FIM,"Checkpoint ICount "FMT_64u"\n",opProcessorICount(PE.processorObj));

    ///Configure next checkpoits
    ///Clear the older ICountBreakpoint (not necessary, but for precaution)
    opProcessorBreakpointICountClear(PE.processorObj);
    Uns64 tempCount = (Uns64) (opProcessorICount(PE.processorObj)/MACRO_CHECKPOINT_INTERVAL);

    ///Create the checkpoint
    sprintf(tempString,"%s/%s/%s-"FMT_64u"",MACRO_APPLICATION_FOLDER,FOLDER_CHECKPOINTS,FILE_NAME_REG_CHECKPOINT,tempCount);
    opProcessorStateSaveFile(PE.processorObj,tempString);

    ///Save the memory
    opMemoryFlush(PE.memObj,0x0,0xFFFFFFFF);
    sprintf(tempString,"%s/%s/%s-"FMT_64u"",MACRO_APPLICATION_FOLDER,FOLDER_CHECKPOINTS,FILE_NAME_MEM_CHECKPOINT,tempCount);
    opMemoryStateSaveFile(PE.memObj,tempString);

    ///Set next ICountBreakpoint
    opProcessorBreakpointICountSet(PE.processorObj,MACRO_CHECKPOINT_INTERVAL);
}

//
// Inject the faults and install the hang callback
//
void handlerFaultInjection(Uns32 processorNumber) {
    char chckName[STRING_SIZE];
    char dumpName[STRING_SIZE];
    char cmpReturn[STRING_SIZE];
    Uns64 tempCount;
    Uns64 memAddress;
    Uns8  memValue;
    Uns64 regTemp=0x0;

    switch(PE.status) {
    case BEFORE_INJECTION: { /// inject the fault
        /// next State
        PE.status = AFTER_INJECTION;

        /// define the Hang condition
        if(MACRO_CHECKPOINT_ENABLE_FLAG) { // !!!! Only valid to one core processor
            if(PE.expectedICount[0] - opProcessorICount(PE.processorObj) < (Uns64) (2*MACRO_CHECKPOINT_INTERVAL) ){ ///Configure the wait for Hang
                ///Configure the next ICountBreakpoint to signal Hangs
                tempCount = (Uns64) ((PE.expectedICount[0] - opProcessorICount(PE.processorObj)) + PE.expectedICount[0]*FIM_HANG_THRESHOLD);
                opProcessorBreakpointICountSet(PE.processorObj,tempCount);
                PE.status = WAIT_HANG;
                opMemoryFlush(PE.memObj,0x0,0xFFFFFFFF);
            } else { ///Wait NEXT CHECKPOINT
                ///Gets the actual Icount and calculate the next ICountBreakpoint
                tempCount = (Uns64) ( MACRO_CHECKPOINT_INTERVAL - opProcessorICount(PE.processorObj) % MACRO_CHECKPOINT_INTERVAL);
                ///Set the next breakpoint
                opProcessorBreakpointICountSet(PE.processorObj,tempCount);
                PE.status = AFTER_INJECTION;
            }
        } else { ///Set the wait to hang for all cores -- including baremetal
            for (int i=0;i<MACRO_NUMBER_OF_CORES;i++) {
                tempCount = (Uns64) ( ( PE.expectedICount[i] - opProcessorICount(PE.childrens[i]) ) + ((PE.expectedICount[i] - PE.applicationBegan[i])*FIM_HANG_THRESHOLD) );
                opProcessorBreakpointICountSet(PE.childrens[i],tempCount);
            }
            PE.status = WAIT_HANG;
        }

        //~ for(int i=0;i<numberOfRegistersToCompare;i++) {
            //~ opProcessorRegReadByName(PE.coreToInject,possibleRegisters[i],&regTemp);
            //~ opPrintf("Value %s "FMT_64x" \n",possibleRegisters[i],regTemp);
        //~ }

        /// insert the fault
        if(MACRO_FAULTTYPE(TYPE_REGISTER) || MACRO_FAULTTYPE(TYPE_FUNCTIONTRACE) || MACRO_FAULTTYPE(TYPE_FUNCTIONTRACE2)) { //Register faults
            // Acquire the register
            opProcessorRegReadByName(PE.coreToInject,PE.faultRegister,&regTemp);
            //~ opPrintf("Reg %s Value "FMT_64x" before "FMT_64x"",PE.faultRegister,PE.faultValue,regTemp);
            // Apply the mask
            regTemp= ~(regTemp ^ PE.faultValue);
            // Writeback register
            opProcessorRegWriteByName(PE.coreToInject,PE.faultRegister,&regTemp);
            //~ opPrintf(" after "FMT_64x" \n",regTemp);

        }else if ( MACRO_FAULTTYPE(TYPE_MEMORY2) || MACRO_FAULTTYPE(TYPE_VARIABLE2) || MACRO_FAULTTYPE(TYPE_FUNCTIONCODE) ){ // Virtual Memory faults
            // Acquire the address
            sscanf(PE.faultTarget,"%lX",&memAddress);
            // Read the VA
            opProcessorRead(PE.childrens[0],memAddress, &memValue, 1, 1, True, OP_HOSTENDIAN_TARGET);
            // Apply the mask
            memValue= ~(memValue ^ (Uns8) PE.faultValue);
            // Write the VA
            opProcessorWrite(PE.childrens[0],memAddress, &memValue, 1, 1, True, OP_HOSTENDIAN_TARGET);
            // Flush memory
            opProcessorFlush(PE.childrens[0],memAddress,memAddress);

        }else if ( MACRO_FAULTTYPE(TYPE_MEMORY)){ // Physical Memory faults
            // Acquire the address
            sscanf(PE.faultTarget,"%lX",&memAddress);
            // Read the Physical Memory
            opMemoryRead(PE.memObj,memAddress,&memValue,1,1);
            // Apply the mask
            memValue= ~(memValue ^ (Uns8) PE.faultValue);
            // Write the Physical Memory
            opMemoryWrite(PE.memObj,memAddress,&memValue,1,1);
            // Flush memory
            opMemoryFlush(PE.memObj,memAddress,memAddress);
        }

        break; }
    case AFTER_INJECTION: { /// Verify if the fault caused an error
        if(MACRO_CHECKPOINT_ENABLE_FLAG) { // !!!! Only valid to one core processor baremetal
            int memCmp=0,regCmp=0;

            /// checkpoint counter
            PE.checkpointCounter++;

            /// configure the next ICountBreakpoint
            /// gets the actual Icount and calculate the next ICountBreakpoint
            tempCount = (Uns64) (MACRO_CHECKPOINT_INTERVAL - opProcessorICount(PE.processorObj) % MACRO_CHECKPOINT_INTERVAL);
            /// set the next breakpoint
            opProcessorBreakpointICountSet(PE.processorObj,tempCount);
            tempCount = (Uns64) (opProcessorICount(PE.processorObj)/MACRO_CHECKPOINT_INTERVAL);

            /// compare the processor state
            sprintf(dumpName,"%s/%s/%s-%u-%u-"FMT_64u"",MACRO_APPLICATION_FOLDER,FOLDER_DUMPS,FILE_NAME_REG_DUMP,MACRO_PLATFORM_ID,processorNumber,tempCount);
            sprintf(chckName,"%s/%s/%s-"FMT_64u"",MACRO_APPLICATION_FOLDER,FOLDER_CHECKPOINTS,FILE_NAME_REG_CHECKPOINT,tempCount);

            opProcessorStateSaveFile(PE.processorObj,dumpName);

            sprintf(tempString,"cmp %s %s ",chckName,dumpName);
            strcpy(cmpReturn,"Null");
            filePointer = popen(tempString, "r");
            if( fgets(cmpReturn,STRING_SIZE,filePointer) != NULL ) opMessage("I",PREFIX_FIM,"Command execution error\n");

            /// remove Dump file to reduce the HD footprint
            sprintf(tempString,"rm %s",dumpName);
            if( system(tempString) ) opMessage("I",PREFIX_FIM,"Command execution error (%s)\n",tempString);

            //~ printf("Comparing if the fault was masked \n");
            //~ printf("REG %s \n",tempString);

            /// compare the popen return with the previous state, if its equal means that the files are equal (i.e. the cmp command didn't return anything)
            if(strcmp(cmpReturn,"Null")==0)
                regCmp = 0;
            else
                regCmp = 1;

            /// compare the memory state
            opMemoryFlush(PE.memObj,0x0,0xFFFFFFFF);
            sprintf(dumpName,"%s/%s/%s-%u-%u-"FMT_64u"",MACRO_APPLICATION_FOLDER,FOLDER_DUMPS,FILE_NAME_MEM_DUMP,MACRO_PLATFORM_ID,processorNumber,tempCount);
            sprintf(chckName,"%s/%s/%s-"FMT_64u"",MACRO_APPLICATION_FOLDER,FOLDER_CHECKPOINTS,FILE_NAME_MEM_CHECKPOINT,tempCount);

            opMemoryStateSaveFile(PE.memObj,dumpName);

            sprintf(tempString,"cmp %s %s ",chckName,dumpName);
            strcpy(cmpReturn,"Null");
            filePointer = popen(tempString, "r");
            if( fgets(cmpReturn,STRING_SIZE,filePointer) != NULL ) opMessage("I",PREFIX_FIM,"Command execution error\n");

            /// remove Dump file to reduce the HD footprint
            sprintf(tempString,"rm %s",dumpName);
            if( !system(tempString) ) opMessage("I",PREFIX_FIM,"Command execution error\n");

            /// compare the popen return with the previous state, if its equal means that the files are equal (i.e. the cmp command didn't return anything)
            if(strcmp(cmpReturn,"Null")==0)
                memCmp = 0;
            else
                memCmp = 1;

            /// no change in the context - Fault was masked
            if(regCmp==0 && memCmp==0) {
                opMessage("I",PREFIX_FIM,"Fault masked\n");
                PE.instructionsToNormalExit=PE.expectedICount[0] - opProcessorICount(PE.processorObj);
                MACRO_UPDATE_STOP_REASON(FIM_SR_MASKED);
                removeFromScheduler(processorNumber);
            }

            /// second count arrived, now wait for the hang
            if((PE.checkpointCounter == 2) || ( (PE.expectedICount[0] - opProcessorICount(PE.childrens[0])) < (Uns64) (2*MACRO_CHECKPOINT_INTERVAL) ) ) {
                tempCount = (Uns64) ((PE.expectedICount[0] - opProcessorICount(PE.childrens[0])) + PE.expectedICount[0]*FIM_HANG_THRESHOLD);
                opProcessorBreakpointICountSet(PE.childrens[0],tempCount);
                PE.status = WAIT_HANG;
            }
        }
        break; }
    case WAIT_HANG: { /// wait for one processor to hang
        opMessage("I",PREFIX_FIM,"Hang detected - processor %d\n",PE.faultIdentifier);
        MACRO_UPDATE_STOP_REASON(FIM_SR_HANG);
        removeFromScheduler(processorNumber);
        break; }
    }
}
