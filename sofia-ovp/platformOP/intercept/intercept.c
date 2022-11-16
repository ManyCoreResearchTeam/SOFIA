// VMI includes
#include "vmi/vmiMessage.h"
#include "vmi/vmiOSAttrs.h"
#include "vmi/vmiOSLib.h"
#include "vmi/vmiRt.h"
#include "vmi/vmiVersion.h"
#include "vmi/vmiInstructionAttrs.h"
#include "vmi/vmiMt.h"

#include "hostapi/impAlloc.h"

#include "op/op.h"
#include "ocl/oclia.h"

#ifndef NULL
    #define NULL ((void*)0)
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
/////////////////////////////////////////////////////////////////////////
// Passing arguments from the ICM PLATFORM
/////////////////////////////////////////////////////////////////////////
#define STRING_SIZE 256

#define OPC_WFI 0xe320f003
#define OPC_WFE 0xe320f002

/////////////////////////////////////////////////////////////////////////
// Structs and Macros
/////////////////////////////////////////////////////////////////////////
#include "../commonStructsAndEnumerators.h"
#include "../commonMacros.h"
#include "icm/icmCpuManager.h"

//Fault Injector header
#include "../options.h"

/////////////////////////////////////////////////////////////////////////
// Variables and prototypes
/////////////////////////////////////////////////////////////////////////
processorDataP processorData;

struct optionsS options;

static VMI_BRANCH_REASON_FN(branchNotifier);
static VMI_PC_WATCH_FN(exitFunc);
static VMI_PC_WATCH_FN(enterFunc);



FILE*  fileTrace;


char   tempString[STRING_SIZE];

// Flags
Uns64           previousInst[4];       // Previous Instruction inside the target block
Uns64           firstInst[4];          // First Instruction inside the target block
Bool            applicationStart=False;


bool   enableBranchNotifier= False; //Check for multicore
//~ bool   enablefunctiontrace = False;
//~ bool   enableVariableTrace = False;

//
// update the stop reason
//
#define MACRO_UPDATE_STOP_REASON(_A) processorData->stopReason=_A;
#define MACRO_FAULTTYPE(_A)           !strcmp(processorData->options.faulttype,faultTypes[_A])

//
// Define the attributes value structure
//
typedef struct paramValuesS {
    VMI_PTR_PARAM(processorData);
} paramValues, *paramValuesP;

//
// This describes a function call context
//
typedef struct callContextS {
    vmiosObjectP object;                // object for this intercept library
    Addr         sp;                    // stack pointer on entry to function
    Uns64        entryICount;           // instruction count on this entry
    Uns32        numCalls;              // number of calls to function
} callContextT, *callContextP;

//
// Define formals
//
static vmiParameter formals[] = {
    VMI_PTR_PARAM_SPEC(paramValues, processorData, 0, "Processor data struct" ),
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

//
// Structure type containing local data for this plugin
//
typedef struct vmiosObjectS {
    //** Function Trace **//
    vmiRegInfoCP result;                // return register (standard ABI)
    vmiRegInfoCP link;                  // link register (standard ABI)
    vmiRegInfoCP sp;                    // stack pointer register (standard ABI)
    Uns32        numCalls;              // number of calls to function
} vmiosObject;

//
// Variable trace object
//
typedef struct variableObjectS {
    bool         readCB;                // read or write CB Flag
    bool         injectfault;           // inject fault Flag
    const char*  variable;              // variable name
    Uns32        numReads;              // number of reads
    Uns32        numWrites;             // number of writes
    Addr         lowAddr;               // variable low address
    Addr         highAddr;              // variable high address
    Uns32        size;                  // variable size in bytes
} variableObject, *variableObjectP;


// Symbols @Review - Make a option
//~ const char processorData->options.tracesymbol[] = "matrixC";

////////////////////////////////////////////////////////////////////////////////
// VARIABLE INTERCEPTION CALLBACKS
////////////////////////////////////////////////////////////////////////////////
//
// Callback when memory written. Add to trace
//
static VMI_MEM_WATCH_FN(traceVariableCB) {

    Uns32 accessValue = 0;
    variableObjectP contextCB = (variableObjectP) userData; /// arguments

    if (processor!=NULL) { /// do not trace the fault injection transaction
        Addr pc = vmirtGetPC(processor); /// get PC

        memDomainP  dataDomain = vmirtGetProcessorDataDomain(processor); /// data domain
        memEndian   endian     = vmirtGetProcessorDataEndian(processor); /// endian

        switch (bytes) { /// number of bytes
            case 1: accessValue = *(Uns8  *)value; break;
            case 2: accessValue = *(Uns16 *)value; break;
            case 4: accessValue = *(Uns32 *)value; break;
            default: break;}

        if (contextCB->readCB) { /// read
            //~ vmiMessage("I",PREFIX_FIM, "Read  variable '%s' @PC 0x"FMT_Ax" PA 0x"FMT_Ax" VA 0x"FMT_Ax" bytes %d value 0x%x \n", contextCB->variable, pc, address, VA, bytes, accessValue); // @Debug
            fprintf(fileTrace,"%3u Read  variable %s @PC 0x"FMT_Ax" PA 0x"FMT_Ax" VA 0x"FMT_Ax" bytes %d value 0x%x \n",contextCB->numReads, contextCB->variable, pc, address, VA, bytes, accessValue);
        } else {                 /// write
            //~ vmiMessage("I",PREFIX_FIM, "Write variable '%s' @PC 0x"FMT_Ax" PA 0x"FMT_Ax" VA 0x"FMT_Ax" bytes %d value 0x%x \n", contextCB->variable, pc, address, VA, bytes, accessValue); // @Debug
            fprintf(fileTrace,"%3u Write variable %s @PC 0x"FMT_Ax" PA 0x"FMT_Ax" VA 0x"FMT_Ax" bytes %d value 0x%x \n",contextCB->numWrites, contextCB->variable, pc, address, VA, bytes, accessValue);
        }

        if(contextCB->injectfault) {
            /// bit flip injection
            if (contextCB->readCB) { /// read
                if(contextCB->numReads==processorData->faultInsertionTime) {
                    accessValue= ~(accessValue ^ processorData->faultValue);
                    vmirtWrite4ByteDomain(dataDomain, VA, endian, accessValue, MEM_AA_FALSE);
                }
            } else { /// write
                if(contextCB->numWrites==processorData->faultInsertionTime) {
                    accessValue= ~(accessValue ^ processorData->faultValue);
                    vmirtWrite4ByteDomain(dataDomain, VA, endian, accessValue, MEM_AA_FALSE);
                }
            }
        }

        /// update the counter
        if (contextCB->readCB) /// read
            contextCB->numReads++;
        else                   /// write
            contextCB->numWrites++;
    }
}

//
// Acquire the contents of a given symbol and returns in a buffer
// TODO: Allocate the buffer internally and automatically?
void getSymbolValue(vmiProcessorP processor, const char* symbolName, void* buffer) {
    memDomainP  dataDomain = vmirtGetProcessorDataDomain(processor); /// data domain

    /// get the function symbol
    vmiSymbolCP funcSymbol = vmirtGetSymbolByName(vmirtGetSMPParent(processor), symbolName);
    Addr funcAddress     = vmirtGetSymbolAddr(funcSymbol);
    Addr funcSize        = vmirtGetSymbolSize(funcSymbol);

    if(funcSymbol) {
        vmiMessage("I",PREFIX_FIM_TRACE_FUNCTION,"Symbol '%s' found (VA:"FMT_64x" SIZE:"FMT_64u") \n",vmirtGetSymbolName(funcSymbol),funcAddress,funcSize);
    } else {
        vmiMessage("F",PREFIX_FIM_TRACE_FUNCTION,"Symbol '%s' not found \n",vmirtGetSymbolName(funcSymbol));
    }

    vmirtReadNByteDomain(dataDomain,funcAddress,buffer,funcSize, 0, MEM_AA_FALSE);
}

////////////////////////////////////////////////////////////////////////////////
// FUNCTION INTERCEPTION CALLBACKS
////////////////////////////////////////////////////////////////////////////////
//
// Called when a return address from a function entry is executed
//
static VMI_PC_WATCH_FN(exitFunc) {
    callContextP context = (callContextP)userData;
    Uns32        stackPointer;

    vmiosRegRead(processor, context->object->sp, &stackPointer);

    /// if stack pointer matches then we have found
    /// the matching return for this context
    if(stackPointer==context->sp) {
        /// get the function result
        Int32 result;
        vmiosRegRead(processor, context->object->result, &result);

        /// report the function return (including elapsed instructions)
        //~ vmiMessage("I",PREFIX_FIM_TRACE_FUNCTION,"Function %s returned with result %d ("FMT_64u" instructions)\n",TRACE_FUNC,result,vmirtGetExecutedICount(processor) - context->entryICount); // @Debug
        //~ vmiMessage("I",PREFIX_FIM_TRACE_FUNCTION, FMT_64u"\n",vmirtGetICount(processor)); // @Debug

        // remove this callback
        vmirtRemovePCCallback(processor, thisPC, exitFunc, context);

        /// free the memory allocated
        free(context);

        fprintf(fileTrace,"C%d-%d EXIT %s CPU %d ICOUNT "FMT_64u"\n",context->numCalls,vmirtGetSMPIndex(processor),processorData->options.tracesymbol,vmirtGetSMPIndex(processor),vmirtGetExecutedICount(processor));
    }
}

//
// Called when the compare function is entered -
// create a new function call context and register a callback on the return address
//
static VMI_PC_WATCH_FN(enterFunc) {
    vmiosObjectP object = (vmiosObjectP)userData;
    Uns32 returnAddress;
    Uns32 stackPointer;

    /// get the address to which the function will return
    vmiosRegRead(processor, object->link, &returnAddress);
    returnAddress -= (returnAddress % 2); ///avoid misaligned address

    /// get the stack pointer on entry, which must match on exit
    vmiosRegRead(processor, object->sp, &stackPointer);

    /// create a new function call context
    callContextP context = STYPE_CALLOC(callContextT);

    /// set info in context
    context->object      = object;
    context->sp          = stackPointer;
    context->entryICount = vmirtGetExecutedICount(processor);
    context->numCalls    = object->numCalls;

    /// add a callback on the return address
    vmirtAddPCCallback(processor, returnAddress, exitFunc, context);

    /// report the function entry
    //~ vmiMessage("I",PREFIX_FIM_TRACE_FUNCTION,"Function %s called with return address 0x%x ("FMT_64u")\n",processorData->options.tracesymbol,returnAddress,vmirtGetExecutedICount(processor)); // @Debug

    /// create report
    fprintf(fileTrace,"C%d-%d ENTER %s CPU %d ICOUNT "FMT_64u"\n",object->numCalls,vmirtGetSMPIndex(processor),processorData->options.tracesymbol,vmirtGetSMPIndex(processor),vmirtGetExecutedICount(processor));

    /// increment count of calls
    object->numCalls++;
}

//
// Trace the function executed instructions
//
static VMI_PC_WATCH_FN(symbolCallback) {
    Uns64  currentInst  = vmirtGetExecutedICount(processor);
    Uns32  smpIndex     = vmirtGetSMPIndex(processor);

    /// The previous instruction was not from the target address space; New block begin
    if ((currentInst-1) != previousInst[smpIndex]) {

        if ( previousInst[smpIndex] != 0) { //First Call, no group need to be closed
            ///Close the previous block
            //~ vmiMessage("I",PREFIX_FIM_TRACE_FUNCTION,"Block cpu %d (S:"FMT_64u" E:"FMT_64u")\n",smpIndex,firstInst[smpIndex],previousInst[smpIndex]); // @Debug
            /// create report
            fprintf(fileTrace,""FMT_64u" "FMT_64u" %d %s \n",firstInst[smpIndex],previousInst[smpIndex],smpIndex,processorData->options.tracesymbol);
        }
        firstInst[smpIndex] = currentInst;
    }

    /// Update pointer
    previousInst[smpIndex] = currentInst;
    /// report the function entry
    //~ vmiMessage("I",PREFIX_FIM_TRACE_FUNCTION,"Function called by cpu %d at address 0x"FMT_64x" ("FMT_64u") -- %s \n",smpIndex,thisPC,currentInst,vmirtProcessorName(processor)); // @Debug
}

////////////////////////////////////////////////////////////////////////////////
// INTERCEPT CALLBACKS
////////////////////////////////////////////////////////////////////////////////
//
// Intercept the FIM Symbols and call the applicationRequestHandler through a vmirtInterrupt
//
static VMIOS_INTERCEPT_FN(serviceHandler) {
    int service = (intptr_t) userData;
    memDomainP  dataDomain = vmirtGetProcessorDataDomain(processor); /// data domain
    memEndian   endian     = vmirtGetProcessorDataEndian(processor); /// endian

    /// read the memory that triggered the CB
    Uns32 checker = vmirtRead4ByteDomain(dataDomain, thisPC, endian, MEM_AA_FALSE);

    /// checking for the correct symbols (0xe320f000 -> NOP operation or 0X4770bf00 ARMv7-M NOP or 0X477046c0 ARMv6-M NOP or 0Xd503201f AARCH64-a NOP )
    if(checker!=0xe320f000 && checker!=0X4770bf00 && checker!=0X477046c0 && checker!=0Xd503201f)
        return;

    /// update the return code
    MACRO_UPDATE_STOP_REASON(service)

    /// enable the tools after the application begin
    switch (service) {
        /// After the application begins
        case  FIM_SR_SERVICE_BEGIN : {
            /// Signal
            applicationStart=True;

            /// Gold execution
            if (processorData->MACRO_GOLD_PLATFORM_FLAG) {
                /// add branch notifier
                if(enableBranchNotifier) {
                    vmirtRegisterBranchNotifier(processor,branchNotifier,0);

                    /// create control flow dump file
                    sprintf(tempString,"./%s/%s-%u",FOLDER_DUMPS,FILE_NAME_TRACE,processorData->faultIdentifier);
                    fileTrace = fopen (tempString,"w");
                }

                /// enable the function trace
                if(MACRO_FAULTTYPE(TYPE_FUNCTIONTRACE)) {
                    /// get the function symbol
                    vmiSymbolCP funcSymbol = vmirtGetSymbolByName(processor, processorData->options.tracesymbol);

                    if(funcSymbol) {
                        /// get the simulated address for the function
                        Addr funcAddress = vmirtGetSymbolAddr(funcSymbol);

                        /// add a callback on the function entry point
                        vmirtAddPCCallback(processor, funcAddress, enterFunc, object);
                        vmiMessage("I",PREFIX_FIM_TRACE_FUNCTION,"Symbol '%s' found \n",processorData->options.tracesymbol);
                    } else {
                        vmiMessage("I",PREFIX_FIM_TRACE_FUNCTION,"Symbol '%s' not found \n",processorData->options.tracesymbol);
                    }

                    /// set up result, link and stack pointer registers for the standard ARM ABI
                    object->result = vmiosGetRegDesc(processor, "R1");
                    object->link   = vmiosGetRegDesc(processor, "LR");
                    object->sp     = vmiosGetRegDesc(processor, "SP");

                    /// create log file with entries and returns
                    sprintf(tempString,"%s",FILE_NAME_TRACE);
                    fileTrace = fopen (tempString,"w");
                }

                /// enable the function trace
                if(MACRO_FAULTTYPE(TYPE_FUNCTIONTRACE2)) {
                    /// get the function symbol
                    vmiSymbolCP funcSymbol;
                    if(vmirtGetSMPParent(processor) != NULL) {
                        funcSymbol = vmirtGetSymbolByName(vmirtGetSMPParent(processor), processorData->options.tracesymbol);
                    } else {
                        funcSymbol = vmirtGetSymbolByName(processor, processorData->options.tracesymbol);
                    }
                    if(funcSymbol) {
                        vmiMessage("I",PREFIX_FIM_TRACE_FUNCTION,"Symbol '%s' found \n",processorData->options.tracesymbol);
                    } else {
                        vmiMessage("F",PREFIX_FIM_TRACE_FUNCTION,"Symbol '%s' not found \n",processorData->options.tracesymbol);
                    }

                    /// data domain
                    Addr funcAddress     = vmirtGetSymbolAddr(funcSymbol);
                    Addr funcSize        = vmirtGetSymbolSize(funcSymbol);

                    if(vmirtGetSMPParent(processor) != NULL){
                        vmiProcessorP parent = vmirtGetSMPParent(processor);

                        /// Install the callback in the function entire extension for all siblings
                        for (vmiProcessorP ch= vmirtGetSMPChild(parent); ch ; ch = vmirtGetSMPNextSibling(ch))
                            for (Addr i=0;i<funcSize;i+=4) {
                                vmirtAddPCCallback(ch, funcAddress+i, symbolCallback, 0);
                            }
                    }
                    else{
                        /// Install the callback in the function entire extension
                        for (Addr i=0;i<funcSize;i+=2) {
                            vmirtAddPCCallback(processor, funcAddress+i, symbolCallback, 0);
                        }
                    }
                    /// create log file with entries and returns
                    sprintf(tempString,"%s",FILE_NAME_TRACE);
                    fileTrace = fopen (tempString,"w");
                }

                /// enable function trace
                if(MACRO_FAULTTYPE(TYPE_VARIABLE)) {
                    Addr highAddr=0,lowAddr=0;
                    Uns32 variableSize = 250000; // @Review - Make a option

                    /// file pointer open
                    sprintf(tempString,"%s",FILE_NAME_TRACE);
                    fileTrace = fopen (tempString,"w");

                    /// data domain
                    memDomainP  dataDomain = vmirtGetProcessorDataDomain(processor);

                    /// get the symbol address (lower)
                    if (vmirtAddressLookup(processor, processorData->options.tracesymbol, &lowAddr) == NULL)
                        vmiMessage("I",PREFIX_FIM,"Symbol Lookup failed for variable '%s'\n",processorData->options.tracesymbol); // @Error
                    else {
                        /// high address
                        highAddr = lowAddr+(variableSize-1);
                        vmiMessage("I",PREFIX_FIM,"Symbol %s found low "FMT_6408x" high "FMT_6408x"\n",processorData->options.tracesymbol,lowAddr,highAddr); // @Info
                    }

                    /// Create a new context
                    variableObjectP contextReadCB  = STYPE_CALLOC(variableObject);
                    variableObjectP contextWriteCB = STYPE_CALLOC(variableObject);

                    /// name
                    contextReadCB->variable  = (const char *)&processorData->options.tracesymbol;
                    contextWriteCB->variable = (const char *)&processorData->options.tracesymbol;

                    /// size in bytes @Review
                    contextReadCB->size  = variableSize;
                    contextWriteCB->size = variableSize;

                    /// low address
                    contextReadCB->lowAddr  = lowAddr;
                    contextWriteCB->lowAddr = lowAddr;

                    /// high address
                    contextReadCB->highAddr  = highAddr;
                    contextWriteCB->highAddr = highAddr;

                    /// read or write CB
                    contextReadCB->readCB  = True;
                    contextWriteCB->readCB = False;

                    /// fault injection
                    if(processorData->MACRO_FI_PLATFORM_FLAG) {
                        if(strcmp(processorData->faultTarget,"Read")) {
                            contextReadCB->injectfault   = True;
                            contextWriteCB->injectfault  = False;
                        } else {
                            contextReadCB->injectfault   = False;
                            contextWriteCB->injectfault  = True;
                        }
                    }

                    /// add callback on the variable address
                    vmirtAddReadCallback (dataDomain, processor, lowAddr, highAddr, traceVariableCB, (void*) contextReadCB);
                    vmirtAddWriteCallback(dataDomain, processor, lowAddr, highAddr, traceVariableCB, (void*) contextWriteCB);
                }

                /// enable variable trace2?
                if(MACRO_FAULTTYPE(TYPE_VARIABLE2) || MACRO_FAULTTYPE(TYPE_FUNCTIONCODE)) {
                    vmiSymbolCP funcSymbol = vmirtGetSymbolByName(vmirtGetSMPParent(processor), processorData->options.tracesymbol);

                    if(funcSymbol) {
                        vmiMessage("I",PREFIX_FIM_TRACE_FUNCTION,"Symbol '%s' found \n",processorData->options.tracesymbol);
                    } else {
                        vmiMessage("F",PREFIX_FIM_TRACE_FUNCTION,"Symbol '%s' not found \n",processorData->options.tracesymbol);
                    }

                    /// data domain
                    Addr funcAddress     = vmirtGetSymbolAddr(funcSymbol);
                    Addr funcSize        = vmirtGetSymbolSize(funcSymbol);
                    //~ vmiPrintf("Here %s "FMT_64u" "FMT_64u" \n",processorData->options.tracesymbol,funcAddress,funcAddress+funcSize);

                    /// create a log file
                    sprintf(tempString,"%s",FILE_NAME_TRACE);
                    fileTrace = fopen (tempString,"w");
                    fprintf(fileTrace,""FMT_64u" "FMT_64u" \n",funcAddress,funcAddress+funcSize);
                    fclose(fileTrace);
                }
            }

        break; }
        /// After the application ends
        case FIM_SR_SERVICE_EXIT : {

            // Reference platform
            if (processorData->MACRO_GOLD_PLATFORM_FLAG) {
                /// Close the trace file
                if(MACRO_FAULTTYPE(TYPE_FUNCTIONTRACE))
                    fclose(fileTrace);

                /// Close the trace file and create the last block
                if(MACRO_FAULTTYPE(TYPE_FUNCTIONTRACE2)) {
                    for (int smpIndex=0; smpIndex<processorData->options.targetcpucores;smpIndex++){
                        fprintf(fileTrace,""FMT_64u" "FMT_64u" %d %s \n",firstInst[smpIndex],previousInst[smpIndex],smpIndex,processorData->options.tracesymbol);
                        //~ vmiMessage("I",PREFIX_FIM_TRACE_FUNCTION,"Block cpu %d (S:"FMT_64u" E:"FMT_64u")\n",smpIndex,firstInst[smpIndex],previousInst[smpIndex]); // @Debug
                    }
                    fclose(fileTrace);
                }

                /// Variable type two
                if(MACRO_FAULTTYPE(TYPE_VARIABLE2)) {
                    vmiSymbolCP funcSymbol = vmirtGetSymbolByName(vmirtGetSMPParent(processor), processorData->options.tracesymbol);
                    Addr funcSize          = vmirtGetSymbolSize(funcSymbol);
                    // Get final the reference result
                    char* buffer = (char*) malloc (sizeof(char) * funcSize);
                    getSymbolValue(processor,processorData->options.tracesymbol,buffer);
                    // Gold file
                    sprintf(tempString,FOLDER_DUMPS"/"FILE_NAME_TRACE"-%d",processorData->MACRO_PLATFORM_ID);
                    fileTrace = fopen (tempString,"wb");
                    // Write
                    fwrite(buffer,sizeof(char),funcSize,fileTrace);
                    fclose(fileTrace);
                }
                // Enable trace to a given variable
                if(processorData->options.tracevariable){
                    vmiSymbolCP varSymbol;
                    // Verify processor type
                    if(vmirtGetSMPParent(processor) != NULL) {
                        // Cortex-A or linux based
                        varSymbol = vmirtGetSymbolByName(vmirtGetSMPParent(processor), processorData->options.tracevariable);
                    } else {
                        // Cortex-M or baremetal
                        varSymbol = vmirtGetSymbolByName(processor, processorData->options.tracevariable);
                    }
                    Addr varSize = vmirtGetSymbolSize(varSymbol);
                    // Get final the reference result
                    char* buffer = (char*) malloc (sizeof(char) * varSize);
                    getSymbolValue(processor,processorData->options.tracevariable,buffer);
                    // Gold file
                    sprintf(tempString,FOLDER_DUMPS"/trace_gold_variable-%d",processorData->MACRO_PLATFORM_ID);
                    fileTrace = fopen (tempString,"w+");
                    // Write
                    //fwrite(buffer,sizeof(char),funcSize,fileTrace);
                    for (int i=0; i<varSize;i++){
                        fprintf(fileTrace,"%02hhX ",buffer[i]);
                    }
                    fclose(fileTrace);
                }

            } else { // Fault Injection Platform

                /// XXXXExecutes in both gold and fault injection platform
                /// Variable type two
                if(MACRO_FAULTTYPE(TYPE_VARIABLE2)) { // Get the target variable and dump the content to later compare
                    Bool SDC   = False;
                    Bool DIRTY = False;

                    // Symbol attributes
                    vmiSymbolCP funcSymbol = vmirtGetSymbolByName(vmirtGetSMPParent(processor), processorData->options.tracesymbol);
                    Addr funcAddress     = vmirtGetSymbolAddr(funcSymbol);
                    Addr funcSize        = vmirtGetSymbolSize(funcSymbol);

                    // Target Address
                    Addr memAddress;
                    sscanf(processorData->faultTarget,"%lX",&memAddress);
                    Uns32 targetIndex = (Uns32) (memAddress-funcAddress);

                    // Buffers
                    char* buffer = (char*) malloc (sizeof(char) * funcSize);
                    char* gold   = (char*) malloc (sizeof(char) * funcSize);

                    // Get Final Result
                    getSymbolValue(processor,processorData->options.tracesymbol,buffer);

                    // Get Gold
                    sprintf(tempString,FOLDER_DUMPS"/"FILE_NAME_TRACE"-0");
                    fileTrace = fopen (tempString,"rb");
                    size_t Result = fread(gold,sizeof(char),funcSize,fileTrace);
                    fclose(fileTrace);
                    if (Result !=funcSize) vmiMessage("I",PREFIX_FIM,"Command execution error\n");

                    vmiPrintf("targetIndex %d\n", targetIndex);

                    // Comparison
                    for (Uns32 i=0; i <funcSize;i++) {
                        vmiPrintf("%d:%02hhX %02hhX \n",i,buffer[i],gold[i]);
                        if (buffer[i] != gold[i]) {
                            if (i == targetIndex) {
                                vmiPrintf("----------- %02hhX %02hhX \n",~(buffer[i] ^ (Uns8) processorData->faultValue),gold[i]);
                                if (  (Uns8) (~(buffer[i] ^ (Uns8) processorData->faultValue)) == (Uns8) gold[i] ) // Dirty variable
                                    { DIRTY = True; vmiPrintf("DIRTY\n");}
                                else
                                    SDC = True; // Mismatch between gold and final result and not in the injected bit
                            } else
                                SDC = True; // Mismatch between gold and final result
                        }
                    }

                    // Dirty bit
                    if (DIRTY) {
                        MACRO_UPDATE_STOP_REASON(FIM_SR_SDC3)
                        vmiPrintf("DIRTY\n");
                    }

                    // Variable mismatch
                    if (SDC) {
                        MACRO_UPDATE_STOP_REASON(FIM_SR_SDC2)
                        vmiPrintf("SDC2\n");
                    }

                    //Dump fault final result
                    //~ sprintf(tempString,FOLDER_DUMPS"/"FILE_NAME_TRACE"-%d",processorData->MACRO_PLATFORM_ID);
                    //~ fileTrace = fopen (tempString,"wb");
                    
                    // Write
                    //~ fwrite(buffer,sizeof(char),funcSize,fileTrace);
                    //~ fclose(fileTrace);
                }
                // Enable trace to a given variable 
                if(processorData->options.tracevariable){
                    vmiSymbolCP varSymbol;
                    // Verify processor type
                    if(vmirtGetSMPParent(processor) != NULL) {
                        // Cortex-A or linux based
                        varSymbol = vmirtGetSymbolByName(vmirtGetSMPParent(processor), processorData->options.tracevariable);
                    } else {
                        // Cortex-M or baremetal
                        varSymbol = vmirtGetSymbolByName(processor, processorData->options.tracevariable);
                    }
                    Addr varSize          = vmirtGetSymbolSize(varSymbol);
                    // Get final the reference result
                    char* buffer = (char*) malloc (sizeof(char) * varSize);
                    getSymbolValue(processor,processorData->options.tracevariable,buffer);
                    // Gold file
                    sprintf(tempString,FOLDER_DUMPS"/trace_fault_variable-%d",processorData->MACRO_PLATFORM_ID);
                    fileTrace = fopen (tempString,"w+");
                    // Write
                    //fwrite(buffer,sizeof(char),funcSize,fileTrace);
                    for (int i=0; i<varSize;i++){
                        fprintf(fileTrace,"%02hhX ",buffer[i]);
                    }
                    fclose(fileTrace);
                }
            }
        break; }

    }

    /// return the opRootModuleSimulate function
    vmirtInterrupt(processor);
}

//
// Branch notifier callback - Trace all control flow changes
//
static VMI_BRANCH_REASON_FN(branchNotifier) {
    // table mapping reason codes to strings
    static const char *reasonStrings[] = {
        [VMIBR_UNCOND]         = "unconditional branch taken",
        [VMIBR_COND_TAKEN]     = "conditional branch taken",
        [VMIBR_COND_NOT_TAKEN] = "conditional branch not taken",
        [VMIBR_PC_SET]         = "PC set by non instruction route",
    };

    Int64 offset = 0;
    ///Get the name of the closest symbol
    const char *symbolName = vmirtSymbolLookup(processor,thisPC,&offset);
    vmiModeInfoCP mode     = vmirtGetCurrentMode(processor);

    fprintf(fileTrace,"REASON (%s 0x"FMT_64u" "FMT_64u"): %38s (prev PC:0x"FMT_A08x" this PC:0x"FMT_A08x") (Symbol %64s 0X%08lx %16s) \n",vmirtProcessorName(processor),vmirtGetExecutedICount(processor),vmirtGetExecutedICount(processor),reasonStrings[reason],thisPC,prevPC,symbolName,offset,mode->name);
}

//
// Runtime callback called for each first instruction in block execution
// Trace the instruction execution
//
static VMIOS_INTERCEPT_FN(firstInstructionBlock) {
    vmiPrintf("REASON (%s): this PC:0x"FMT_Ax"\n", vmirtProcessorName(processor), thisPC);
}

//
// Morph time callback called before each instruction is morphed
//
VMIOS_MORPH_FN(instrTrace) {
    // Only doing transparent interceptions
    *opaque = False;

    if(firstInBlock)
        return firstInstructionBlock;

    /// Instruction tracing is not active
    return NULL;
}

/////////////////////////////////////////////////////////////////////////
// Constructor and destructor
/////////////////////////////////////////////////////////////////////////
//
// Constructor
//
static VMIOS_CONSTRUCTOR_FN(constructor) {
    paramValuesP params = parameterValues;
    processorData = (processorDataP) params->processorData;
}

//
// Destructor
//
static VMIOS_DESTRUCTOR_FN(destructor) {
    //~ if(enableBranchNotifier)
        //~ fclose(fileTrace);

    //~ if(processorData->options.enablefunctiontrace ||  processorData->options.enablefunctiontrace2)
        //~ fclose(fileTrace);

    //~ if(MACRO_FAULTTYPE(TYPE_VARIABLE))
        //~ fclose(fileTrace);

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
    .packageName   = "faultinjection",       // description
    .objectSize    = sizeof(vmiosObject),    // size in bytes of OSS object

    ////////////////////////////////////////////////////////////////////////
    // CONSTRUCTOR/DESTRUCTOR ROUTINES
    ////////////////////////////////////////////////////////////////////////

    .constructorCB = constructor,            // object constructor
    .destructorCB  = destructor,             // object destructor
    .paramSpecsCB  = getParamSpecs,          // iterate parameter declarations
    ////////////////////////////////////////////////////////////////////////
    // ADDRESS INTERCEPT DEFINITIONS
    ////////////////////////////////////////////////////////////////////////

    // INSTRUCTION INTERCEPT ROUTINES
    //~ .morphCB        = instrTrace,                // morph callback cpu_v7_do_idle

    // ADDRESS INTERCEPT DEFINITIONS
    .intercepts = {
        // -----------------        ------- ------ --------          ---------
        // Name                     Address Opaque Callback          userData
        // -----------------        ------- ------ --------          ---------
        {"__ovp_init",              0,      False, serviceHandler, (void*) FIM_SR_SERVICE_BEGIN},
        {"__ovp_exit",              0,      False, serviceHandler, (void*) FIM_SR_SERVICE_EXIT},
        {"__ovp_segfault",          0,      False, serviceHandler, (void*) FIM_SR_SERVICE_SEG_FAULT},
        {"__ovp_init_external",     0,      False, serviceHandler, (void*) FIM_SR_SERVICE_BEGIN},
        {"__ovp_segfault_external", 0,      False, serviceHandler, (void*) FIM_SR_SERVICE_SEG_FAULT},
        { 0 },
    }
};

