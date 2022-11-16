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


#include <string.h>

// VMI area includes
#include "vmi/vmiMessage.h"
#include "vmi/vmiOSAttrs.h"
#include "vmi/vmiRt.h"
#include "vmi/vmiDoc.h"

#include "hostapi/impAlloc.h"

// local includes
#include "svc.h"
#include "memory.h"

// iGen includes
#include "intercept.igen.h"

#define UNUSED __attribute__((unused))

//
//  Morph time Callback function
//
VMIOS_MORPH_FN(ceMorphCB) {

    VMI_ASSERT(processor == object->processor, "Object processor mis-match");

    object->simulationStarted = True;

    // if not enabled do nothing
    if(!object->enabled) {
        return NULL;
    }

    VMI_ASSERT(object->procMorphCB, "Processor-specific morph callback not set");

    return object->procMorphCB(
            processor, object, thisPC, inDelaySlot, firstInBlock,
            emitTrace, opaque, context, userData
            );
}

//
// Update stretch time - call this at least once per quantum
//
void updateStretchTime(vmiosObjectP object) {

    Uns64 addedCycles = object->additionalCycles - object->prevAdditionalCycles;

    if (addedCycles) {

        if(DIAG_MED) {
            vmiMessage(
                    "I", PREFIX "_STC",
                    "%s: icount " FMT_64u": Stretch time by " FMT_64u ,
                    object->name,
                    vmirtGetExecutedICount(object->processor),
                    addedCycles
                    );
        }

        vmirtAddSkipCount(object->processor, addedCycles, 0);

    }

    object->prevAdditionalCycles = object->additionalCycles;

}

//
// Branch notifier callback for handling stretch feature
// TODO: Replace with end of quantum notifier when that API is available
//
VMI_BRANCH_REASON_FN(branchStretchNotifier) {

    vmiosObjectP object = userData;

    if(object->stretch) {

        // Only update every Nth branch if stretchTimeGranularity is specified
        // (this is to minimize the number of VMI stretch calls)
        object->branchCounter++;
        if (object->branchCounter >= object->stretchTimeGranularity) {

            updateStretchTime(object);

            object->branchCounter = 0;

        }
    }
}


/////////////////////////////////////////////
// Constructor and Destructor functions
/////////////////////////////////////////////

// License name
#define LICENSE_NAME       "IMP_VAP"

//
// Check for required licenses
//
static void licenseCheck(vmiosObjectP object) {

    if (!vmiosGetLicenseFeature(LICENSE_NAME)) {
        vmiMessage(
                "F", PREFIX"_UOL",
                "%s: Unable to obtain license:\n%s\n"
                "Please contact Imperas to obtain the necessary license\n",
                object->name,
                vmiosGetLicenseFeatureErrString(LICENSE_NAME)
                );
    }
}

void addLocalDocumentation(vmiosObjectP object) {
    vmiDocNodeP root = vmidocAddSection(0, "Root");
    vmiDocNodeP desc = vmidocAddSection(root, "Description");

    // Description
    vmidocAddText(desc, "CPU Estimator");
    vmidocAddText(desc, "Estimates the timing for an application execution based upon timing analysis of instructions groups "
            " and using algorithms for previous instructions executed and branch prediction.");

    vmiDocNodeP lic = vmidocAddSection(root, "Licensing");
    vmidocAddText(lic,"Imperas Commercial/Proprietary License");

    vmiDocNodeP ref = vmidocAddSection(root, "Reference");
    vmiDocNodeP lim = vmidocAddSection(root, "Limitations");

    // general Limitations (applicable to all)
    vmidocAddText(lim,"Still under development and subject to change without notice.");

    // TODO: How to make type/variant documentation work?
    // Does the intercept library need to be able to supply a list
    // of processor types and variants that it supports?

    // We are specifying a specific variant
    if(strcmp(object->type, "armm") == 0) {
        if(strcmp(object->variant, "Cortex-M4F") == 0) {
            vmidocAddText(ref,"Cortex-M4F Reference");
            vmidocAddText(ref,"Hardware analysis performed using STM32F4 Discovery Kit.");
            vmidocAddText(lim,"TBA");
        } else {
            vmidocAddText(ref,"armm Reference TBD");
            vmidocAddText(lim,"armm Limitations TBD");
        }
    } else if (strcmp(object->type, "mips32") == 0) {
        if(strcmp(object->variant, "M5150") == 0) {
            vmidocAddText(ref,"M5150 Reference");
        } if(strcmp(object->variant, "M5100") == 0) {
            vmidocAddText(ref,"M5100 Reference");
        }

        vmidocAddText(ref,"MIPS32 Reference TBD");
        vmidocAddText(lim,"Branch prediction is estimated");
    } else if (strcmp(object->type, "v850") == 0) {
        vmidocAddText(ref,"V850 Reference TBD");
        vmidocAddText(lim,"Branch prediction TBA");
    }

    // add documenation to processor
    vmidocProcessor(object->processor, root);
}

//
// Object constructor
//
static VMIOS_CONSTRUCTOR_FN(ceConstructor) {

    formalValuesP params = parameterValues;

    // Initialize the vmiosObject
    object->processor  = processor;
    object->endian     = vmirtGetProcessorDataEndian(processor);
    object->dataDomain = vmirtGetProcessorDataDomain(processor);
    object->name       = strdup(vmirtProcessorName(processor));

    object->defaultCount = 0;
    object->branchCount = 0;
    object->memoryCount = 0;
    object->memoryLoadCount = 0;
    object->memoryStoreCount = 0;
    object->arithLogicCount = 0;
    object->arithLogicMovCount = 0;
    object->multDivCount = 0;
    object->fpCount = 0;
    object->fpMovCount = 0;
    object->SIMDarithLogicCount = 0;
    object->SIMDarithLogicMovCount = 0;
    object->SIMDvec_arithLogicCount = 0;
    object->SIMDsca_arithLogicCount = 0;
    object->SIMDvec_multCount = 0;
    object->SIMDsca_multCount = 0;
    object->SIMDsca_fpCount = 0;
    object->SIMDvec_tblCount = 0;
    object->SIMDvec_memoryLoadCount = 0;
    object->SIMDvec_memoryStoreCount = 0;
    object->SIMDcryptoCount = 0;
    object->systemCount = 0;

    object->regX0 = 0;
    object->regX1 = 0;
    object->regX2 = 0;
    object->regX3 = 0;
    object->regX4 = 0;
    object->regX5 = 0;
    object->regX6 = 0;
    object->regX7 = 0;
    object->regX8 = 0;
    object->regX9 = 0;
    object->regX10 = 0;
    object->regX11 = 0;
    object->regX12 = 0;
    object->regX13 = 0;
    object->regX14 = 0;
    object->regX15 = 0;
    object->regX16 = 0;
    object->regX17 = 0;
    object->regX18 = 0;
    object->regX19 = 0;
    object->regX20 = 0;
    object->regX21 = 0;
    object->regX22 = 0;
    object->regX23 = 0;
    object->regX24 = 0;
    object->regX25 = 0;
    object->regX26 = 0;
    object->regX27 = 0;
    object->regX28 = 0;
    object->regX29 = 0;
    object->regX30 = 0;
    object->regSp = 0;
    object->regPc = 0;
    object->regLr = 0;
    object->regD0 = 0;
    object->regD1 = 0;
    object->regD2 = 0;
    object->regD3 = 0;
    object->regD4 = 0;
    object->regD5 = 0;
    object->regD6 = 0;
    object->regD7 = 0;
    object->regD8 = 0;
    object->regD9 = 0;
    object->regD10 = 0;
    object->regD11 = 0;
    object->regD12 = 0;
    object->regD13 = 0;
    object->regD14 = 0;
    object->regD15 = 0;
    object->regD16 = 0;
    object->regD17 = 0;
    object->regD18 = 0;
    object->regD19 = 0;
    object->regD20 = 0;
    object->regD21 = 0;
    object->regD22 = 0;
    object->regD23 = 0;
    object->regD24 = 0;
    object->regD25 = 0;
    object->regD26 = 0;
    object->regD27 = 0;
    object->regD28 = 0;
    object->regD29 = 0;
    object->regD30 = 0;
    object->regD31 = 0;
    object->regOthers = 0;

    object->iCount = 0;

    object->prevICount = 0;
    object->watchReg = False;

    // TODO: Need better processor type definition? This is user defined when
    //       icmNewProcessor is used.
    object->type       = strdup(vmirtProcessorType(processor));

    // variant may be NULL
    const char * variant = vmirtProcessorVariant(processor);
    if(variant) {
        object->variant   = strdup(variant);
    }

    if (params) {
        object->stretchTimeGranularity = params->stretchTimeGranularity;
    }

    addLocalDocumentation(object);

    licenseCheck(object);

    constructParser(object, &object->cmdArgs);

}

#define FREE(_p) {if (_p) {free(_p); _p = NULL;}}

//
// Destructor called when terminating simulation
//
static VMIOS_DESTRUCTOR_FN(ceDestructor) {

    if (object->procDestructor) {
        (*object->procDestructor)(object);
    }

    memPenaltyP this = object->firstMemPenalty;
    while (this) {
        memPenaltyP next = this->next;
        this->next = NULL;
        free(this);
        this = next;
    }
    object->firstMemPenalty = NULL;

    FREE (object->name);

    FREE (object->type);

    FREE (object->filename);

}

////////////////////////////////////////////////////////////////////////////////
// INTERCEPT ATTRIBUTES
////////////////////////////////////////////////////////////////////////////////

vmiosAttr modelAttrs = {

    ////////////////////////////////////////////////////////////////////////
    // VERSION
    ////////////////////////////////////////////////////////////////////////

    .versionString  = VMI_VERSION,                // version string (THIS MUST BE FIRST)
    .modelType      = VMI_INTERCEPT_LIBRARY,      // type
    .packageName    = PLUGIN_PREFIX,              // description
    .objectSize     = sizeof(vmiosObject),        // size in bytes of OSS object

    .releaseStatus  = VMI_INTERNAL,               // Currently internal only

    ////////////////////////////////////////////////////////////////////////
    // VLNV INFO
    ////////////////////////////////////////////////////////////////////////

    .vlnv = {
        .vendor  = "imperas.internal.com",
        .library = "intercept",
        .name    = "cpuEstimator",
        .version = "1.0"
    },

    ////////////////////////////////////////////////////////////////////////
    // CONSTRUCTOR/DESTRUCTOR ROUTINES
    ////////////////////////////////////////////////////////////////////////

    .constructorCB  = ceConstructor,   // object constructor
    .destructorCB   = ceDestructor,    // object destructor

    ////////////////////////////////////////////////////////////////////////
    // INSTRUCTION INTERCEPT ROUTINES
    ////////////////////////////////////////////////////////////////////////

    .morphCB        = ceMorphCB,                  // morph callback
    .nextPCCB       = 0,                          // get next instruction address
    .disCB          = 0,                          // disassemble instruction

    ////////////////////////////////////////////////////////////////////////
    // PARAMETER CALLBACKS
    ////////////////////////////////////////////////////////////////////////

    .paramSpecsCB     = nextParameter,          // iterate parameter declarations
    .paramValueSizeCB = getTableSize,           // get parameter table size

    ////////////////////////////////////////////////////////////////////////
    // ADDRESS INTERCEPT DEFINITIONS
    ////////////////////////////////////////////////////////////////////////
    // -------------------   ----------- ------ -----------------
    // Name                  Addr        Opaque Callback
    // -------------------   ----------- ------ -----------------
    .intercepts = {
        {0}
    }
};
