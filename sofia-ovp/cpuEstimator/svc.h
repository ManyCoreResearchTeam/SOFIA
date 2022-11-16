/*
 *
 * Copyright (c) 2005-2016 Imperas Software Ltd., All Rights Reserved.
 *
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND TRADE SECRETS
 * OF IMPERAS SOFTWARE LTD. USE, DISCLOSURE, OR REPRODUCTION IS PROHIBITED
 * EXCEPT AS MAY BE PROVIDED FOR IN A WRITTEN AGREEMENT WITH
 * IMPERAS SOFTWARE LTD.
 *
 */

#ifndef _TIMINGTOOLS_SVC_H
#define _TIMINGTOOLS_SVC_H

#ifndef PREFIX
#define PREFIX "CPUEST"
#endif

#define PLUGIN_PREFIX PREFIX
#define CPU_PREFIX PREFIX

#include "vap/vapTypes.h"

#include "memory.h"

#include "intercept.igen.h"

// Processor-specific enable function prototype
#define PROC_ENABLE_FN(_NAME) void _NAME(vmiosObjectP  object)
typedef PROC_ENABLE_FN((*procEnableFn));

// Declarations of processor-specific enable functions
// These are responsible for setting all other processor-specific function pointers
PROC_ENABLE_FN(armCEEnable);
PROC_ENABLE_FN(armmCEEnable);
PROC_ENABLE_FN(rh850CEEnable);
PROC_ENABLE_FN(m5100CEEnable);

// Processor-specific destructor function prototype
#define PROC_DESTRUCTOR_FN(_NAME) void _NAME(vmiosObjectP  object)
typedef PROC_DESTRUCTOR_FN((*procDestructorFn));

// stretch time call back to be registered when cycle estimation is enabled
// TODO: Replace with end of quantum notifier when that API is available
VMI_BRANCH_REASON_FN(branchStretchNotifier);

typedef struct vmiosObjectS {

    // Set by constructor
    vmiProcessorP processor;
    char       *name;
    char       *variant;
    char       *type;
    memEndian   endian;
    memDomainP  dataDomain;

    // Additional cycle count count estimation (beyond one cycle per instruction)
    // Updated by processor-specific and memory functions
    Uns64 additionalCycles;

    Uns64 defaultCount;
    Uns64 branchCount;
    Uns64 memoryCount;
    Uns64 memoryLoadCount;
    Uns64 memoryStoreCount;
    Uns64 arithLogicCount;
    Uns64 arithLogicMovCount;
    Uns64 multDivCount;
    Uns64 fpCount;
    Uns64 fpMovCount;
    Uns64 SIMDarithLogicCount;
    Uns64 SIMDarithLogicMovCount;
    Uns64 SIMDvec_arithLogicCount;
    Uns64 SIMDsca_arithLogicCount;
    Uns64 SIMDvec_multCount;
    Uns64 SIMDsca_multCount;
    Uns64 SIMDsca_fpCount;
    Uns64 SIMDvec_tblCount;
    Uns64 SIMDvec_memoryLoadCount;
    Uns64 SIMDvec_memoryStoreCount;
    Uns64 SIMDcryptoCount;
    Uns64 systemCount;

    Uns64 regX0;
    Uns64 regX1;
    Uns64 regX2;
    Uns64 regX3;
    Uns64 regX4;
    Uns64 regX5;
    Uns64 regX6;
    Uns64 regX7;
    Uns64 regX8;
    Uns64 regX9;
    Uns64 regX10;
    Uns64 regX11;
    Uns64 regX12;
    Uns64 regX13;
    Uns64 regX14;
    Uns64 regX15;
    Uns64 regX16;
    Uns64 regX17;
    Uns64 regX18;
    Uns64 regX19;
    Uns64 regX20;
    Uns64 regX21;
    Uns64 regX22;
    Uns64 regX23;
    Uns64 regX24;
    Uns64 regX25;
    Uns64 regX26;
    Uns64 regX27;
    Uns64 regX28;
    Uns64 regX29;
    Uns64 regX30;

    Uns64 regSp;
    Uns64 regPc;
    Uns64 regLr;

    Uns64 regD0;
    Uns64 regD1;
    Uns64 regD2;
    Uns64 regD3;
    Uns64 regD4;
    Uns64 regD5;
    Uns64 regD6;
    Uns64 regD7;
    Uns64 regD8;
    Uns64 regD9;
    Uns64 regD10;
    Uns64 regD11;
    Uns64 regD12;
    Uns64 regD13;
    Uns64 regD14;
    Uns64 regD15;
    Uns64 regD16;
    Uns64 regD17;
    Uns64 regD18;
    Uns64 regD19;
    Uns64 regD20;
    Uns64 regD21;
    Uns64 regD22;
    Uns64 regD23;
    Uns64 regD24;
    Uns64 regD25;
    Uns64 regD26;
    Uns64 regD27;
    Uns64 regD28;
    Uns64 regD29;
    Uns64 regD30;
    Uns64 regD31;

    Uns64 regOthers;

    Uns64 iCount;
    Uns64 prevICount;
    const char *dis;

    Bool watchReg;

    // Stretch settings
    Uns32 stretchTimeGranularity;   // number of branches to skip between stretch calls
    Uns32 branchCounter;            // Count skipped branches between stretch calls
    Uns64 prevAdditionalCycles;     // additional cycle count at last stretch call

    // Command and simulation state set by cmd/morph functions
    Bool        simulationStarted;   // simulation has begun
    Bool        enabled;             // cycle estimation enabled
    Bool        stretch;             // stretch enabled
    char       *filename;            // user-specified name for CE results file
    Uns64	faultIcount;
    Bool 	active;
    char 	*targetRegister;
    memPenaltyP firstMemPenalty;     // head of list of memPenalty structures

    // Processor-specific functions
    procEnableFn     procEnable;        // Called when CE enabled by user command
    vmiosMorphFn     procMorphCB;       // Called by morph call back function
    procDestructorFn procDestructor;    // Called at destruction time

    // Pointer for processor-specific data structure
    void *procCEData;

    // Command arguments (required by iGen generated functions)
    cmdArgValues  cmdArgs;

    // Pipeline stalls(mips32)
    Uns64 possibleStalledCylces;
    Uns64 lastInstructionIssue;
    Uns32 lastInstructionIclass;


} vmiosObject;

#define DIAG_LEVEL(_O) ((_O)->cmdArgs.diagnosticStr.level)

#define DIAG_LOW    (DIAG_LEVEL(object) > 0)
#define DIAG_MED    (DIAG_LEVEL(object) > 1)
#define DIAG_HIGH   (DIAG_LEVEL(object) > 2)

#ifdef DEBUG
#define DIAG_DBG    (DIAG_LEVEL(object) > 99)
#else
#define DIAG_DBG    (0)
#endif

#endif // _TIMINGTOOLS_SVC_H
