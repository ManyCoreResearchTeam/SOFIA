/////////////////////////////////////////////////////////////////////////
// Structs and enumerators
/////////////////////////////////////////////////////////////////////////

#define END_LIST "EndList@"
char  possibleRegisters[50][10];
int   numberOfRegistersToCompare;
const char possibleRegistersV7[][10] ={"r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12","sp","lr","pc","d0","d1","d2","d3","d4","d5","d6","d7","d8","d9","d10","d11","d12","d13","d14","d15",END_LIST};
const char possibleRegistersV8[][10] ={"x0","x1","x2","x3","x4","x5","x6","x7","x8","x9","x10","x11","x12","x13","x14","x15","x16","x17","x18","x19","x20","x21","x22","x23","x24","x25","x26","x27","x28","x29","x30","sp","pc","v0","v1","v2","v3","v4","v5","v6","v7","v8","v9","v10","v11","v12","v13","v14","v15","v16","v17","v18","v19","v20","v21","v22","v23","v24","v25","v26","v27","v28","v29","v30","v31",END_LIST};
const char possibleRegistersRV[][10] ={"ra","sp","gp","tp","t0","t1","t2","s0","s1","a0","a1","a2","a3","a4","a5","a6","a7","s2","s3","s4","s5","s6","s7","s8","s9","s10","s11","t3","t4","t5","t6","pc",END_LIST};

#include "options.h"

//
// Internal struct of each PE
//
typedef struct processorStruct {
    /// simulator objects
    optProcessorP   processorObj;
    optProcessorP*  childrens;
    optMemoryP      memObj;
    optBusP         busObj;
    optModuleP      modObj;

    /// core to inject the fault
    optProcessorP coreToInject;

    /// fault injection
    Uns32  faultIdentifier;
    //~ Uns32  faultType;
    Uns32  status;

    /// fault mask
    Uns64  faultValue;
    
    /// faulty register information
    char   faultRegister[STRING_SIZE];
    Uns32  faultRegisterPosition;
    
    char   faultTypeStr[STRING_SIZE];
    char   faultTarget[STRING_SIZE];

    /// fault injection states
    Uns32  stopReason;
    Uns32  checkpointCounter;
    Uns64  checkpoint;

    /// options
    struct optionsS options;

    /// number of instruction to insert the fault
    Uns64 faultInsertionTime;
    /// expected number of instructions for each core
    Uns64* expectedICount;
    /// point which the application began
    Uns64* applicationBegan;
    /// point which the application end
    Uns64* applicationEnd;
    /// number of instructions that will be saved
    Uns64 instructionsToNormalExit;
    /// not more used - make persistent faults
    Uns64 faultDuration;
} processorDataS,*processorDataP;

///Extension for opStopReasonE OP_SR_INVALID = 0x13
enum {
    FIM_SR_MASKED = 0x14,               // Masked faults
    FIM_SR_UNSET,                       // Default and unset error
    FIM_SR_Hard_Fault,                  // Hard fault handler triggered
    FIM_SR_LOCKUP,                      // Lockup error (only for M3/M4)
    FIM_SR_HANG,                        // Hang detection
    FIM_SR_SDC,                        // Hang detection
    FIM_SR_SDC2,                        // Hang detection
    FIM_SR_SDC3,                        // Hang detection
    FIM_SR_DMA_ERROR,                   // DMA Error for use the NOC
    FIM_SR_SEG_FAULT,                   // Segmentation Fault
    FIM_SR_ILLEGAL_INST,                // Illegal instruction
    FIM_SR_UNIDENTIFIED,                // Unidentified error
    FIM_SR_SERVICE_EXIT,                // Service -- Exit the simulation
    FIM_SR_SERVICE_BEGIN,               // Service -- Application began
    FIM_SR_SERVICE_SEG_FAULT,           // Service -- Segmentation fault
    FIM_SR_SERVICE_EXTERNAL_EXIT,       // Service from the external caller -- Exit the simulation
    FIM_SR_SERVICE_EXTERNAL_BEGIN,      // Service from the external caller -- Application began
    FIM_SR_SERVICE_EXTERNAL_SEG_FAULT   // Service from the external caller -- Segmentation fault
};

///Simulation States
enum {
    GOLD_PROFILE,
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
    FIM_SERVICE_SEG_FAULT
};

///Final error classification
enum {
    Masked_Fault,                   // Masked fault
    Control_Flow_Data_OK,           // Divergency between executed insctructions and gold, memory correct
    Control_Flow_Data_ERROR,        // Divergency between executed insctructions and gold, memory incorrect
    REG_STATE_Data_OK,              // Internal state incorrect and memory no
    REG_STATE_Data_ERROR,           // Internal state and memory incorrect
    SDC,                            // Silent Data Corruption
    SDC2,                           // Variable corrupted
    SDC3,                           // Variable Dirty
    RD_PRIV,                        // Read privilege exception.
    WR_PRIV,                        // Write privilege exception.
    RD_ALIGN,                       // Read align exception.
    WR_ALIGN,                       // Write align exception.
    FE_PRIV,                        // Fetch privilege exception.
    FE_ABORT,                       // Fetch an inconsistent instruction
    SEG_FAULT,                      // Segmentation fault in the OS
    ARITH,                          // Arithmetic exception.
    Hard_Fault,                     // Hard Fault handler
    Lockup,                         // Cortex M4 special state
    Unknown,                        // Unknown
    Hang,                           // Application presumably hanged
    SIZE_ERROR_CLASS_LIST           // Size of this list
};

///The name of the error classes, only keep the same order
const char* const errorNames[] = {
    "Masked_Fault",
    "Control_Flow_Data_OK",
    "Control_Flow_Data_ERROR",
    "REG_STATE_Data_OK",
    "REG_STATE_Data_ERROR",
    "silent_data_corruption",
    "silent_data_corruption2",
    "silent_data_corruption3",
    "RD_PRIV",
    "WR_PRIV",
    "RD_ALIGN",
    "WR_ALIGN",
    "FE_PRIV",
    "FE_ABORT",
    "SEG_FAULT",
    "ARITH",
    "Hard_Fault",
    "Lockup",
    "Unknown",
    "Hang",
    "SIZE_ERROR_CLASS_LIST"};

/// fault type enumerator and names
enum {
    TYPE_REGISTER,
    TYPE_MEMORY,
    TYPE_MEMORY2,
    TYPE_ROBSOURCE,
    TYPE_ROBDEST,
    TYPE_VARIABLE,
    TYPE_VARIABLE2,
    TYPE_FUNCTIONTRACE,
    TYPE_FUNCTIONTRACE2,
    TYPE_FUNCTIONCODE
};

const char* const faultTypes[] = {
    "register",
    "memory",
    "memory2",
    "robsource",
    "robdest",
    "variable",
    "variable2",
    "functiontrace",
    "functiontrace2",
    "functioncode"
    };
