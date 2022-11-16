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

#ifndef MEMORY_H
#define MEMORY_H

#include "vmi/vmiTypes.h"

// Structure used as userData for memCB calls
typedef struct memPenaltyS {
    vmiosObjectP        object;
    Addr                low;
    Addr                high;
    Uns32               cycles;
    struct memPenaltyS *next;
} memPenaltyT, *memPenaltyP;

//
// Called on access to memory flagged with a memory access penalty
//
VMI_MEM_WATCH_FN(memoryCyclesCB);

#endif /* MEMORY_H */
