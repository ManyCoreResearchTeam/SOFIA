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

// VMI area includes

#include "vmi/vmiTyperefs.h"
#include "vmi/vmiRt.h"

// local includes
#include "svc.h"
#include "memory.h"

////////////////////////////////////////////////////////////////////////////////
// Memory callbacks
////////////////////////////////////////////////////////////////////////////////

//
// Called on access to memory flagged with a memory access penalty
//
VMI_MEM_WATCH_FN(memoryCyclesCB) {

    memPenaltyP  memPenalty = (memPenaltyP) userData;
    vmiosObjectP object     = memPenalty->object;

    // Only penalize non-artifact accesses made by this processor
    if (processor == object->processor) {

        if (DIAG_MED) {
            vmiMessage(
                    "I", PREFIX "_CAM",
                    "%s: %d byte access at 0x"FMT_A08x": %d memory penalty cycles added\n",
                    object->name,
                    bytes,
                    address,
                    memPenalty->cycles
                    );
        }

        object->additionalCycles += memPenalty->cycles;

    }
}
