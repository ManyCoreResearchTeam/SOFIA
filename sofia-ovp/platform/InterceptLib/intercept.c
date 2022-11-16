// VMI area includes
#include "vmi/vmiMessage.h"
#include "vmi/vmiOSAttrs.h"
#include "vmi/vmiOSLib.h"
#include "vmi/vmiRt.h"
#include "vmi/vmiVersion.h"

#ifndef NULL
    #define NULL ((void*)0)
#endif

/////////////////////////////////////////////////////////////////////////
// Passing arguments from the ICM PLATFORM
/////////////////////////////////////////////////////////////////////////
#define STRING_SIZE 256
#include "icm/icmCpuManager.h"

#define OPC_WFI 0xe320f003
#define OPC_WFE 0xe320f002

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
        
        Uns64 instructionsToNormalExit;
        ///Not more used - make persistent faults
        Uns64 faultDuration;
} NodeData;

struct processorStruct* vectorProcessor;

//
// Define the attributes value structure
//
typedef struct paramValuesS {
    VMI_PTR_PARAM(vectorProcessor);
} paramValues, *paramValuesP;

//
// Define formals
//
static vmiParameter formals[] = {
    VMI_PTR_PARAM_SPEC(paramValues, vectorProcessor,   0, "Processor data struct" ),
    VMI_END_PARAM
};

//
// Iterate formals
//
static VMIOS_PARAM_SPEC_FN(getParamSpecs) {
    if(!prev) {
        return formals;
    } else {
        prev++;
        if (prev->name) {
            return prev;
        } else {
            return NULL;
        }
    }
}

/////////////////////////////////////////////////////////////////////////

//
// Structure type containing local data for this plugin
//
typedef struct vmiosObjectS {
    Uns32 totalIntercepts;
} vmiosObject;

//
// Constructor
//
static VMIOS_CONSTRUCTOR_FN(constructor) {
    paramValuesP params = parameterValues;
    vectorProcessor = (struct processorStruct*) params->vectorProcessor;
}

//
// Destructor
//
static VMIOS_DESTRUCTOR_FN(destructor) {

}

////////////////////////////////////////////////////////////////////////////////
// INTERCEPT CALLBACKS
////////////////////////////////////////////////////////////////////////////////
//
// Common callback for all interceptions
//
//~ static VMIOS_INTERCEPT_FN(doFTrace) {

    //~ vmiPrintf(
        //~ "*** ftrace-plugin: cpu %s calls %s (after " FMT_64u " instructions)\n",
        //~ vmirtProcessorName(processor),
        //~ context,
        //~ vmirtGetICount(processor)
    //~ );
    
    //vmirtInterrupt(processor);
    //~ object->totalIntercepts++;
//~ }

//
// Opcode data structure to be passed as userdata on instruction interception
//
typedef struct opcodeDataS {
    Uns32 opcode;       // Opcode value
    Uns8  size;         // Opcode size in bytes
} opcodeDataT, *opcodeDataP, **opcodeDataPP;

//
// Runtime callback called for each instruction execution
// Trace the instruction execution
//
static VMIOS_INTERCEPT_FN(traceInstruction) {
    vmiPrintf("ID %d - Name %s - Core %u\n",vectorProcessor->faultIdentifier,vmirtProcessorName(processor),vmirtGetSMPIndex(processor));
    //~ Uns32 coreIndex = vmirtGetSMPIndex(processor);
    
    //~ if(coreIndex == 0) { //The main core should never sleep unless its is an error
        //~ vectorProcessor->StopReason = FIM_SR_Hang;
        //~ vmirtInterrupt(processor);
    //~ }
}

//
// Get the opcode data at thisPC
//
static void getOpcode( vmiProcessorP processor, vmiosObjectP object, Addr thisPC, opcodeDataP opcodeP) {

    Uns32 opcode = 0;
    Uns32 bytes  = vmirtInstructionBytes(processor, thisPC);

    switch (bytes) {
    case 2:
        opcode = (Uns32) vmicxtFetch2Byte(processor, thisPC);
        break;
    case 4:
        opcode = vmicxtFetch4Byte(processor, thisPC);
        break;
    default:
        vmiPrintf("Invalid instruction size %d for instruction at 0x"FMT_A08x,  bytes, thisPC);
        break;
    }
    
    opcodeP->opcode=opcode;
    opcodeP->size  = bytes;
}

//
// Morph time callback called before each instruction is morphed
//
VMIOS_MORPH_FN(instrTrace) {

    // Only doing transparent interceptions
    *opaque = False;

    // Get the instruction opcode structure
    opcodeDataT opcodeT = { .opcode=0 , .size=0, }; 
    getOpcode(processor, object, thisPC, &opcodeT);

    //Get WFE and WFI instructions
    if (opcodeT.opcode == OPC_WFE) {
        vmiPrintf("OPCODE: %u \n",opcodeT.opcode);
        // Pass the opcode to the instruction trace call back
        //*userData = (void *) opcode;
        
        // Add a transparent callback for the instruction
        return traceInstruction;
    }

    // Instruction tracing is not active
    return NULL;

}

////////////////////////////////////////////////////////////////////////////////
// INTERCEPT ATTRIBUTES
////////////////////////////////////////////////////////////////////////////////
vmiosAttr modelAttrs = {

    ////////////////////////////////////////////////////////////////////////
    // VERSION
    ////////////////////////////////////////////////////////////////////////

    .versionString = VMI_VERSION,            // version string
    .modelType     = VMI_INTERCEPT_LIBRARY,  // type
    .packageName   = "FIM-plugin",           // description
    .objectSize    = sizeof(vmiosObject),    // size in bytes of OSS object

    ////////////////////////////////////////////////////////////////////////
    // CONSTRUCTOR/DESTRUCTOR ROUTINES
    ////////////////////////////////////////////////////////////////////////

    .constructorCB = constructor,            // object constructor
    .destructorCB  = destructor,             // object destructor
    .paramSpecsCB  = getParamSpecs,       // iterate parameter declarations
    ////////////////////////////////////////////////////////////////////////
    // ADDRESS INTERCEPT DEFINITIONS
    ////////////////////////////////////////////////////////////////////////

    // INSTRUCTION INTERCEPT ROUTINES
    .morphCB        = instrTrace,                // morph callback

    // ADDRESS INTERCEPT DEFINITIONS
    .intercepts = { {0} }
    //~ .intercepts = {
        //~ // ----------------- ------- ------ --------  ---------
        //~ // Name              Address Opaque Callback  userData
        //~ // ----------------- ------- ------ --------  ---------
        //~ {  "main",              0,      False, doFTrace, 0  },
        //~ {  "bpnn_train_kernel", 0,      False, doFTrace, 0  },
        //~ {  "_start",            0,      False, doFTrace, 0  },
        //~ {  "__ovp_exit",        0,      False, doFTrace, 0  },
        //~ {  0                                                },
    //~ }
};

