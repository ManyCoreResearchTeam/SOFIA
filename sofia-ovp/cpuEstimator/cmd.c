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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vmi/vmiMessage.h"
#include "vmi/vmiRt.h"

#include "hostapi/impAlloc.h"

#include "svc.h"
#include "memory.h"

#include "intercept.macros.igen.h"

// Table of processor type/variants with support for Cycle Estimation
typedef struct supportedProcessorsS {
    const char  *procType;      // value returned by vmirtProcessorType
    const char  *procVariant;   // value returned by vmirtProcessorVariant
    procEnableFn enableFn;
} supportedProcessorsT, *supportedProcessorsP;

static supportedProcessorsT procList[] = {
    { "armm",  "Cortex-M0", armCEEnable  },
    { "armm",  "Cortex-M3", armCEEnable  },
    { "armm",  "Cortex-M4", armCEEnable  },
    { "armm",  "Cortex-M4F", armCEEnable  },
    { "armm",  "Cortex-M7", armCEEnable  },
    { "armm",  "Cortex-M7F", armCEEnable  },
    { "arm",  "Cortex-A53MPx1", armCEEnable  },
    { "arm",  "Cortex-A57MPx1", armCEEnable  },
    { "arm",  "Cortex-A72MPx1", armCEEnable  },
    { "arm",  "Cortex-A72MPx2", armCEEnable  },
    { "arm",  "Cortex-A72MPx4", armCEEnable  },
    { "arm",  "Cortex-A9MPx1",    armCEEnable  },
    { "arm",  "Cortex-A5MPx1",    armCEEnable  },
    { "arm",  "Cortex-A7MPx1",    armCEEnable  },
//    { "riscv", "RISCV32I", rh850CEEnable },
    { 0, 0, 0 }
};

//
// Look for the processor/variant defined in object and return the procEnableFn
// for it. Returns NULL if none found
//
static procEnableFn getEnableFunction(vmiosObjectP object) {

    supportedProcessorsP p;

    for (p=procList; p->procType != NULL; p++) {
        if ((strcmp(object->type,    p->procType   ) == 0) &&
                (strcmp(object->variant, p->procVariant) == 0)
           ) {
            return p->enableFn;
        }
    }

    return NULL;

}

// Helper macros for accessing arguments parsed by iGen generated functions
#define ENABLED(_O)    ((_O)->cmdArgs.instTraceStr.on)
#define STRETCH(_O)    ((_O)->cmdArgs.instTraceStr.stretch)
#define FILENAME(_O)   ((_O)->cmdArgs.instTraceStr.filename)
#define FAULT(_O)      ((_O)->cmdArgs.instTraceStr.faultIcount)
#define ACTIVE(_O)      ((_O)->cmdArgs.instTraceStr.active)
#define REGISTER(_O)      ((_O)->cmdArgs.instTraceStr.targetRegister)
#define MEMHIGH(_O)    ((_O)->cmdArgs.memorycyclesStr.high)
#define MEMLOW(_O)     ((_O)->cmdArgs.memorycyclesStr.low)
#define MEMPENALTY(_O) ((_O)->cmdArgs.memorycyclesStr.penalty)

#define ITRACE_CMD "instTrace"

VMIOS_COMMAND_PARSE_FN(instTraceCB) {

    // Should always be called with exactly argc arguments
    VMI_ASSERT(
            argc==(AI_instTrace_LAST),
            "Incorrect arguments for command %s: expected %d, got %d",
            ITRACE_CMD, AI_instTrace_LAST, argc
            );

    procEnableFn procEnableFunction = getEnableFunction(object);

    /*if (object->simulationStarted) {

      vmiMessage(
      "E", PREFIX"_SHB",
      "%s: %s command ignored: Command may not be used after simulation has begun.",
      object->name, ITRACE_CMD
      );

      } else */
    if (!procEnableFunction) {

        vmiMessage(
                "E", PREFIX"_PNS",
                "%s: %s command ignored: processor type '%s' with variant '%s' is not supported. "
                "Contact Imperas info@imperas.com",
                object->name,
                ITRACE_CMD,
                object->type,
                object->variant ? object->variant : "no variant"
                );

    } else {

        object->procEnable = procEnableFunction;
        object->enabled    = ENABLED(object);
        object->stretch    = STRETCH(object);
        object->faultIcount  = FAULT(object);
        object->active 	   = ACTIVE(object);

        if (argv[AI_instTrace_filename].isSet) {
            if (object->filename) free(object->filename);
            object->filename = strdup(FILENAME(object));
        }

        if (argv[AI_instTrace_targetRegister].isSet) {
            if (object->targetRegister) free(object->targetRegister);
            object->targetRegister = strdup(REGISTER(object));
        }

        if (object->enabled) {

            // Call the processor-specific enable function
            (*object->procEnable)(object);

            // Register the branch notifier for stretch option
            // TODO: Replace with end of quantum notifier when that API is available
            // FELIPE: maybe vmirtCreateModelTimer(object->processor,callbackStretch,100,object);
            if (object->stretch) {
                vmirtRegisterBranchNotifier(object->processor, branchStretchNotifier, object);
            }
        }
    }

    vmiMessage (
            "I", PREFIX"_CMD",
            "%s: %s %s%s%s%s",
            object->name,
            ITRACE_CMD,
            object->enabled ? "on" : "off",
            object->stretch ? ": time stretch enabled" : "",
            object->filename ? ": results will be written to file " : "",
            object->filename ? object->filename : ""
            );

    return "";

}

//////////////////////////// memorycycles command //////////////////////////////

#define MEMORYCYCLES_CMD "memorycycles"

VMIOS_COMMAND_PARSE_FN(memorycyclesCB) {

    Addr low  = MEMLOW(object);
    Addr high = MEMHIGH(object);

    if (low > high) {
        vmiMessage(
                "E", PREFIX"_IAR",
                "%s: %s command ignored: low address 0x"FMT_A08x" is greater than high address 0x"FMT_A08x,
                object->name,
                MEMORYCYCLES_CMD,
                low,
                high
                );

    } else {

        memPenaltyP memPenalty = STYPE_CALLOC(memPenaltyT);

        memPenalty->object      = object;
        memPenalty->low         = low;
        memPenalty->high        = high;
        memPenalty->cycles      = MEMPENALTY(object);
        memPenalty->next        = object->firstMemPenalty;
        object->firstMemPenalty = memPenalty;

        vmiMessage (
                "I", PREFIX"_MEM",
                "%s: %s: Memory access penalty of %d cycles set for address range 0x"FMT_Ax":0x"FMT_AX,
                object->name,
                MEMORYCYCLES_CMD,
                memPenalty->cycles,
                memPenalty->low, memPenalty->high
                );

        vmirtAddReadCallback (
                object->dataDomain, object->processor,
                low, high,
                memoryCyclesCB, memPenalty
                );
        vmirtAddWriteCallback(
                object->dataDomain, object->processor,
                low, high,
                memoryCyclesCB, memPenalty
                );

    }

    return "";

}


//////////////////////////////// Diagnostic Command /////////////////////////////////

VMIOS_COMMAND_PARSE_FN(diagnosticCB) {

    VMI_ASSERT(
            argc==(AI_diagnostic_LAST),
            "Incorrect argc: expected %d, got %d",
            AI_diagnostic_LAST, argc
            );

    if (DIAG_LOW) {
        vmiMessage(
                "I", PREFIX"_DLS", "%s: diagnostic level set to %d",
                object->name,
                DIAG_LEVEL(object)
                );
    }

    return "";

}
