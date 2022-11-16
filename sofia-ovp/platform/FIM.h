#ifndef _FIM
#define _FIM

/*
 * Fault Injection Version - 3.01
 */
  
/*************************************************************************************************
* Important, is defined PE as vectorProcessor[processorNumber]
**************************************************************************************************/

//~ #define _GNU_SOURCE

///System includes
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stddef.h>
#include <cstddef>
#include <algorithm>


///Parser Options
#include "OptionParser.h"
#define IsGoldExecution (bool)options.get("FIM_gold_run")
#define IsFaultCampaignExecution (bool)options.get("FIM_fault_campaign_run")
#define PlatformIdentifier (int)options.get("FIM_platform_ID")
#define NumberOfRunningFaults (int)options.get("FIM_number_of_faults")
#define NumberOfRunningCores (int)options.get("FIM_number_of_Cores")

#define CheckpointInterval (int) options.get("FIM_checkpoint_profile")
#define IsCheckpointEnable ((int)options.get("FIM_checkpoint_profile")!=0)

#define IsLinuxMode !options["FIM_Mode"].compare("Linux")
#define IsBareMetalMode !options["FIM_Mode"].compare("BareMetal")
#define LinuxSuportFolder options["FIM_linux_folder"].c_str()
#define ApplicationFolder options["FIM_project_folder"].c_str()

#define FIM_HANG_THRESHOLD 0.5


///Classification categories and registers definition
#include "Classification.h"

#ifndef NULL
    #define NULL ((void*)0)
#endif

///Local includes from OVPSim
#include "icm/icmCpuManager.hpp"

///FIM Defines
#define GOLD_PLATFORM -1
#define LOAD_FAULTS 1

///PE wrapper
#define PE vectorProcessor[processorNumber]

///Extension for icmStopReasonE ICM_SR_INVALID = 0x13
enum {
    FIM_SR_Masked = 0x14,
    FIM_SR_Unset,
    FIM_SR_Hard_Fault,
    FIM_SR_Lockup,
    FIM_SR_Hang,
    FIM_SR_DMA_ERROR,
    FIM_SR_SEG_FAULT,
    FIM_SR_ILLEGAL_INST,
    FIM_SR_UNIDENTIFIED,
};

///Simulation States
enum {
    GOLD_INJECTION,
    BEFORE_INJECTION,
    AFTER_INJECTION,
    WAIT_HANG,
    CREATE_FAULT,
    NO_INJECTION,
    FORCING_FAULT
};

///OVP-FIM App Services
enum {
    FIM_SERVICE_BEGIN_APP,
    FIM_SERVICE_END_APP,
    FIM_SERVICE_SEG_FAULT,
    FIM_SERVICE_EXIT_APP
};

///External memory to control the OVP-FIM
const unsigned int baseAddress = 0xFA000000;
const unsigned int len = 0x0FF;

///* Internal struct of each PE */
typedef struct processorStruct {
        ///Description
        Uns32 id;
        Uns32 faultIdentifier;
        
        ///Simulator objects
        icmProcessorP   processorObj;
        icmProcessorP*  childrens;
        icmMemoryP      memObj;
        ///core to inject the fault
        icmProcessorP coreToInject;
        
        ///Fault injection
        Uns32  faultType;
        Uns32  Status;
        
        ///Fault Mask
        Uns32 faultValue;
        
        char faultRegister[STRING_SIZE];
        Uns32  faultRegisterPosition;
        
        ///Fault Injection states
        Uns32  StopReason;
        Uns32  CHCKCount;
        Uns64  CHCKpoint;
        
        ///Number of instruction to insert the fault
        Uns64 faultPos;
        ///Expected number of instructions for each core
        Uns64* expectedICount;
        ///Point which the application began 
        Uns64* ApplicationBegan;
        Uns64* ApplicationEnd;
        
        Uns64 instructionsToNormalExit;
        ///Not more used - make persistent faults
        Uns64 faultDuration;
} processor, NodeData;

/** ------------------------------------------------------------------------------------- **/

void report();
void updateStopReason(int stopReason,int processorNumber);
void parseArguments(int argc, char **argv);
void initPlatform();
void initProcessorInstFIM( Uns32 processorNumber, icmProcessorP processorObj, icmMemoryP  memObj);
//~ int  compareFiles(char* file1, char* file2);
void setFault(Uns32 faultType, Uns32 processorNumber);
void loadFaults();
void setProfiling(Uns32 processorNumber);
void YieldAllCores(Uns32 processorNumber);
void FaultInjection(Uns32 processorNumber);
void GoldProfiling(Uns32 processorNumber);
void LockupExcepetion (void *arg, Uns32 value);
void handleWatchpoints(void);

ICM_MEM_WATCH_FN(faultInjector);

/** ------------------------------------------------------------------------------------- **/    
#endif
