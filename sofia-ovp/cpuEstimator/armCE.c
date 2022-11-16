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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <string.h>

// VMI area includes
#include "vmi/vmiInstructionAttrs.h"
#include "vmi/vmiMessage.h"
#include "vmi/vmiMt.h"
#include "vmi/vmiRt.h"

// Imperas common library includes
#include "ocl/oclia.h"

// Utility includes
#include "hostapi/impAlloc.h"
#include "treeSearch.h"

// local includes
#include "svc.h"

// Defines
#define SIZEOFINBITS(_A) sizeof(_A) * 8

// Instruction classes with different cycle estimation algorithms
typedef enum {
    IC_default,
    IC_initial,
    IC_branch,
    IC_memory,
    IC_memoryLoad,
    IC_memoryStore,
    IC_arithLogic,
    IC_arithLogicMov,
    IC_multDiv,
    IC_fp,
    IC_fpMov,
    IC_SIMD_arithLogic,
    IC_SIMD_arithLogicMov,
    IC_SIMD_vec_arithLogic,
    IC_SIMD_sca_arithLogic,
    IC_SIMD_vec_mult,
    IC_SIMD_sca_mult,
    IC_SIMD_sca_fp,
    IC_SIMD_vec_tbl,
    IC_SIMD_vec_memoryLoad,
    IC_SIMD_vec_memoryStore,
    IC_SIMD_crypto,
    IC_system,
} instrClassesE;

// Struct for model specific data for arm processor instance
typedef struct armCEDataS {
    instrClassesE prevClassRT;  // Previous instruction's class (run time)
    instrClassesE prevClassMT;  // Previous instruction's class (morph time)
    vmiRegInfoCP regDestMT;  // previous instruction's destination register
    // (morph time)

    vmiRegInfoCP cpsr;  // Processor's cpsr register

} armCEDataT, *armCEDataP;

////////////////////////////////////////////////////////////////////////////////
// Global Variables
// Note these are common to and shared by all instances of the library
////////////////////////////////////////////////////////////////////////////////

// Table matching instruction memonic to associated class
typedef struct instructionTableS {
    instrClassesE instructionClass;
    const char *mnemonic;
} instructionTableT, *instructionTableP, **instructionTablePP;

static instructionTableT instructionClasses[] = {

    // Conditional Branch
    {IC_branch, "b.eq"},
    {IC_branch, "b.ne"},
    {IC_branch, "b.cs"},
    {IC_branch, "b.hs"},
    {IC_branch, "b.cc"},
    {IC_branch, "b.lo"},
    {IC_branch, "b.mi"},
    {IC_branch, "b.pl"},
    {IC_branch, "b.vs"},
    {IC_branch, "b.vc"},
    {IC_branch, "b.hi"},
    {IC_branch, "b.ls"},
    {IC_branch, "b.ge"},
    {IC_branch, "b.lt"},
    {IC_branch, "b.gt"},
    {IC_branch, "b.le"},
    {IC_branch, "b.al"},
    {IC_branch, "b.nv"},
    {IC_branch, "cbnz"},
    {IC_branch, "cbz"},
    {IC_branch, "tbnz"},
    {IC_branch, "tbz"},

    // Unconditional Branch
    {IC_branch, "b"},
    {IC_branch, "bl"},
    {IC_branch, "blr"},
    {IC_branch, "br"},
    {IC_branch, "ret"},

    // Memory load

    {IC_memoryLoad, "ldr"},
    {IC_memoryLoad, "ldrb"},
    {IC_memoryLoad, "ldrsb"},
    {IC_memoryLoad, "ldrh"},
    {IC_memoryLoad, "ldrsh"},
    {IC_memoryLoad, "ldrsw"},
    {IC_memoryStore, "str"},
    {IC_memoryStore, "strb"},
    {IC_memoryStore, "strh"},
    {IC_memoryLoad, "ldur"},
    {IC_memoryLoad, "ldurb"},
    {IC_memoryLoad, "ldursb"},
    {IC_memoryLoad, "ldurh"},
    {IC_memoryLoad, "ldursh"},
    {IC_memoryLoad, "ldursw"},
    {IC_memoryStore, "stur"},
    {IC_memoryStore, "sturb"},
    {IC_memoryStore, "sturh"},
    {IC_memoryLoad, "ldp"},
    {IC_memoryLoad, "ldpsw"},
    {IC_memoryStore, "stp"},
    {IC_memoryLoad, "ldnp"},
    {IC_memoryStore, "stnp"},
    {IC_memoryLoad, "ldtr"},
    {IC_memoryLoad, "ldtrb"},
    {IC_memoryLoad, "ldtrsb"},
    {IC_memoryLoad, "ldtrh"},
    {IC_memoryLoad, "ldtrsh"},
    {IC_memoryLoad, "ldtrsw"},
    {IC_memoryStore, "sttr"},
    {IC_memoryStore, "sttrb"},
    {IC_memoryStore, "sttrh"},
    {IC_memoryLoad, "ldxr"},
    {IC_memoryLoad, "ldxrb"},
    {IC_memoryLoad, "ldxrh"},
    {IC_memoryLoad, "ldxp"},
    {IC_memoryStore, "stxr"},
    {IC_memoryStore, "stxrb"},
    {IC_memoryStore, "stxrh"},
    {IC_memoryStore, "stxp"},
    {IC_memoryLoad, "ldar"},
    {IC_memoryLoad, "ldarb"},
    {IC_memoryLoad, "ldarh"},
    {IC_memoryStore, "stlr"},
    {IC_memoryStore, "stlrb"},
    {IC_memoryStore, "stlrh"},
    {IC_memoryLoad, "ldaxr"},
    {IC_memoryLoad, "ldaxrb"},
    {IC_memoryLoad, "ldaxrh"},
    {IC_memoryLoad, "ldaxp"},
    {IC_memoryStore, "stlxr"},
    {IC_memoryStore, "stlxrb"},
    {IC_memoryStore, "stlxrh"},
    {IC_memoryStore, "stlxp"},
    {IC_memory, "prfm"},
    {IC_memory, "prfum"},

    // Data Processing
    {IC_arithLogic, "add"},
    {IC_arithLogic, "adds"},
    {IC_arithLogic, "sub"},
    {IC_arithLogic, "subs"},
    {IC_arithLogic, "cmp"},
    {IC_arithLogic, "cmn"},
    {IC_arithLogicMov, "mov"},
    {IC_arithLogic, "and"},
    {IC_arithLogic, "ands"},
    {IC_arithLogic, "eor"},
    {IC_arithLogic, "orr"},
    {IC_arithLogicMov, "movi"},
    {IC_arithLogic, "tst"},
    {IC_arithLogicMov, "movz"},
    {IC_arithLogicMov, "movn"},
    {IC_arithLogicMov, "movk"},
    {IC_arithLogic, "adrp"},
    {IC_arithLogic, "adr"},
    {IC_arithLogic, "bfm"},
    {IC_arithLogic, "sbfm"},
    {IC_arithLogic, "ubfm"},
    {IC_arithLogic, "bfi"},
    {IC_arithLogic, "bfxil"},
    {IC_arithLogic, "sbfiz"},
    {IC_arithLogic, "sbfx"},
    {IC_arithLogic, "ubfiz"},
    {IC_arithLogic, "ubfx"},
    {IC_arithLogic, "extr"},
    {IC_arithLogic, "asr"},
    {IC_arithLogic, "lsl"},
    {IC_arithLogic, "lsr"},
    {IC_arithLogic, "ror"},
    {IC_arithLogic, "sxt"},
    {IC_arithLogic, "uxt"},
    {IC_arithLogic, "neg"},
    {IC_arithLogic, "negs"},
    {IC_arithLogic, "bic"},
    {IC_arithLogic, "bics"},
    {IC_arithLogic, "eon"},
    {IC_arithLogic, "orn"},
    {IC_arithLogic, "mvn"},
    {IC_arithLogic, "asrv"},
    {IC_arithLogic, "lslv"},
    {IC_arithLogic, "lsrv"},
    {IC_arithLogic, "rorv"},
    {IC_arithLogic, "cls"},
    {IC_arithLogic, "clz"},
    {IC_arithLogic, "rbit"},
    {IC_arithLogic, "rev16"},
    {IC_arithLogic, "rev32"},
    {IC_arithLogic, "rev"},
    {IC_arithLogic, "adc"},
    {IC_arithLogic, "adcs"},
    {IC_arithLogic, "csel"},
    {IC_arithLogic, "csinc"},
    {IC_arithLogic, "csinv"},
    {IC_arithLogic, "csneg"},
    {IC_arithLogic, "cset"},
    {IC_arithLogic, "csetm"},
    {IC_arithLogic, "cinc"},
    {IC_arithLogic, "cinv"},
    {IC_arithLogic, "cneg"},
    {IC_arithLogic, "sbc"},
    {IC_arithLogic, "sbcs"},
    {IC_arithLogic, "ngc"},
    {IC_arithLogic, "ngcs"},
    {IC_arithLogic, "ccmn"},
    {IC_arithLogic, "ccmp"},
    {IC_arithLogic, "sxtb"},
    {IC_arithLogic, "sxth"},
    {IC_arithLogic, "sxtw"},
    {IC_arithLogic, "uxtb"},
    {IC_arithLogic, "uxth"},

    // Multiply / Divide
    {IC_multDiv, "madd"},
    {IC_multDiv, "msub"},
    {IC_multDiv, "mneg"},
    {IC_multDiv, "mul"},
    {IC_multDiv, "smaddl"},
    {IC_multDiv, "smsubl"},
    {IC_multDiv, "smnegl"},
    {IC_multDiv, "smull"},
    {IC_multDiv, "smulh"},
    {IC_multDiv, "umaddl"},
    {IC_multDiv, "umsubl"},
    {IC_multDiv, "umnegl"},
    {IC_multDiv, "umull"},
    {IC_multDiv, "umulh"},
    {IC_multDiv, "sdiv"},
    {IC_multDiv, "udiv"},

    // Floating Point
    {IC_fpMov, "fmov"},
    {IC_fp, "fcvt"},
    {IC_fp, "fcvtas"},
    {IC_fp, "fcvtau"},
    {IC_fp, "fcvtms"},
    {IC_fp, "fcvtmu"},
    {IC_fp, "fcvtns"},
    {IC_fp, "fcvtnu"},
    {IC_fp, "fcvtps"},
    {IC_fp, "fcvtpu"},
    {IC_fp, "fcvtzs"},
    {IC_fp, "fcvtzu"},
    {IC_fp, "scvtf"},
    {IC_fp, "ucvtf"},
    {IC_fp, "frinta"},
    {IC_fp, "frinti"},
    {IC_fp, "frintm"},
    {IC_fp, "frintn"},
    {IC_fp, "frintp"},
    {IC_fp, "frintx"},
    {IC_fp, "frintz"},
    {IC_fp, "fabs"},
    {IC_fp, "fneg"},
    {IC_fp, "fsqrt"},
    {IC_fp, "fadd"},
    {IC_fp, "fdiv"},
    {IC_fp, "fmul"},
    {IC_fp, "fnmul"},
    {IC_fp, "fsub"},
    {IC_fp, "fmax"},
    {IC_fp, "fmaxnm"},
    {IC_fp, "fmin"},
    {IC_fp, "fminnm"},
    {IC_fp, "fmadd"},
    {IC_fp, "fmsub"},
    {IC_fp, "fnmadd"},
    {IC_fp, "fnmsub"},
    {IC_fp, "fcmp"},
    {IC_fp, "fcmpe"},
    {IC_fp, "fccmp"},
    {IC_fp, "fccmpe"},
    {IC_fp, "fcsel"},

    // Advanced SIMD

    /* { IC_SMID_               , "5.7.3"     }, */
    {IC_SIMD_arithLogic, "dup"},
    {IC_SIMD_arithLogic, "ins"},
    /* { IC_SMID_arithLogic     , "mov"       }, */
    {IC_SIMD_arithLogicMov, "smov"},
    {IC_SIMD_arithLogicMov, "umov"},

    /* { IC_SMID_               , "5.7.4"     }, */
    /* { IC_SMID_vec_arithLogic , "add"       }, */
    /* { IC_SMID_vec_arithLogic , "and"       }, */
    /* { IC_SMID_vec_arithLogic , "bic"       }, */
    {IC_SIMD_vec_arithLogic, "bif"},
    {IC_SIMD_vec_arithLogic, "bit"},
    {IC_SIMD_vec_arithLogic, "bsl"},
    /* { IC_SMID_vec_arithLogic , "cmeq"      }, */
    /* { IC_SMID_vec_arithLogic , "cmge"      }, */
    /* { IC_SMID_vec_arithLogic , "cmgt"      }, */
    /* { IC_SMID_vec_arithLogic , "cmhi"      }, */
    /* { IC_SMID_vec_arithLogic , "cmhs"      }, */
    /* { IC_SMID_vec_arithLogic , "cmle"      }, */
    /* { IC_SMID_vec_arithLogic , "cmlo"      }, */
    /* { IC_SMID_vec_arithLogic , "cmls"      }, */
    /* { IC_SMID_vec_arithLogic , "cmlt"      }, */
    /* { IC_SMID_vec_arithLogic , "cmtst"     }, */
    /* { IC_SMID_vec_arithLogic , "eor"       }, */
    /* { IC_SMID_vec_arithLogic , "fabd"      }, */
    /* { IC_SMID_vec_arithLogic , "facge"     }, */
    /* { IC_SMID_vec_arithLogic , "facgt"     }, */
    /* { IC_SMID_vec_arithLogic , "facle"     }, */
    /* { IC_SMID_vec_arithLogic , "faclt"     }, */
    /* { IC_SMID_vec_arithLogic , "fadd"      }, */
    /* { IC_SMID_vec_arithLogic , "fcmeq"     }, */
    /* { IC_SMID_vec_arithLogic , "fcmge"     }, */
    /* { IC_SMID_vec_arithLogic , "fcmgt"     }, */
    /* { IC_SMID_vec_arithLogic , "fcmle"     }, */
    /* { IC_SMID_vec_arithLogic , "fcmlt"     }, */
    /* { IC_SMID_vec_arithLogic , "fdiv"      }, */
    /* { IC_SMID_vec_arithLogic , "fmax"      }, */
    /* { IC_SMID_vec_arithLogic , "fmaxnm"    }, */
    /* { IC_SMID_vec_arithLogic , "fmin"      }, */
    /* { IC_SMID_vec_arithLogic , "fminnm"    }, */
    /* { IC_SMID_vec_arithLogic , "fmla"      }, */
    /* { IC_SMID_vec_arithLogic , "fmls"      }, */
    /* { IC_SMID_vec_arithLogic , "fmul"      }, */
    /* { IC_SMID_vec_arithLogic , "fmulx"     }, */
    /* { IC_SMID_vec_arithLogic , "frecps"    }, */
    /* { IC_SMID_vec_arithLogic , "frsqrts"   }, */
    /* { IC_SMID_vec_arithLogic , "fsub"      }, */
    /* { IC_SMID_vec_arithLogic , "mla"       }, */
    /* { IC_SMID_vec_arithLogic , "mls"       }, */
    /* { IC_SMID_vec_arithLogic , "mul"       }, */
    /* { IC_SMID_vec_arithLogic , "orn"       }, */
    /* { IC_SMID_vec_arithLogic , "orr"       }, */
    {IC_SIMD_vec_arithLogic, "pmul"},
    {IC_SIMD_vec_arithLogic, "saba"},
    {IC_SIMD_vec_arithLogic, "sabd"},
    {IC_SIMD_vec_arithLogic, "shadd"},
    {IC_SIMD_vec_arithLogic, "shsub"},
    {IC_SIMD_vec_arithLogic, "smax"},
    {IC_SIMD_vec_arithLogic, "smin"},
    /* { IC_SMID_vec_arithLogic , "sqadd"     }, */
    /* { IC_SMID_vec_arithLogic , "sqdmulh"   }, */
    /* { IC_SMID_vec_arithLogic , "sqrdmulh"  }, */
    /* { IC_SMID_vec_arithLogic , "sqrshl"    }, */
    /* { IC_SMID_vec_arithLogic , "sqshl"     }, */
    /* { IC_SMID_vec_arithLogic , "sqsub"     }, */
    {IC_SIMD_vec_arithLogic, "srhadd"},
    /* { IC_SMID_vec_arithLogic , "srshl"     }, */
    /* { IC_SIMD_vec_arithLogic , "sshl"      }, */
    /* { IC_SMID_vec_arithLogic , "sub"       }, */
    {IC_SIMD_vec_arithLogic, "uaba"},
    {IC_SIMD_vec_arithLogic, "uabd"},
    {IC_SIMD_vec_arithLogic, "uhadd"},
    {IC_SIMD_vec_arithLogic, "uhsub"},
    {IC_SIMD_vec_arithLogic, "umax"},
    {IC_SIMD_vec_arithLogic, "umin"},
    /* { IC_SMID_vec_arithLogic , "uqrshl"    }, */
    /* { IC_SMID_vec_arithLogic , "uqshl"     }, */
    /* { IC_SMID_vec_arithLogic , "uqsub"     }, */
    {IC_SIMD_vec_arithLogic, "urhadd"},
    /* { IC_SMID_vec_arithLogic , "urshl"     }, */
    /* { IC_SMID_vec_arithLogic , "ushl"      }, */

    /* { IC_SMID_               , "5.7.5"     }, */
    /* { IC_SMID_sca_arithLogic , "add"       }, */
    {IC_SIMD_sca_arithLogic, "cmeq"},
    {IC_SIMD_sca_arithLogic, "cmge"},
    {IC_SIMD_sca_arithLogic, "cmgt"},
    {IC_SIMD_sca_arithLogic, "cmhi"},
    {IC_SIMD_sca_arithLogic, "cmhs"},
    {IC_SIMD_sca_arithLogic, "cmle"},
    {IC_SIMD_sca_arithLogic, "cmlo"},
    {IC_SIMD_sca_arithLogic, "cmls"},
    {IC_SIMD_sca_arithLogic, "cmlt"},
    {IC_SIMD_sca_arithLogic, "cmtst"},
    {IC_SIMD_sca_arithLogic, "fabd"},
    {IC_SIMD_sca_arithLogic, "facge"},
    {IC_SIMD_sca_arithLogic, "facgt"},
    {IC_SIMD_sca_arithLogic, "facle"},
    {IC_SIMD_sca_arithLogic, "faclt"},
    {IC_SIMD_sca_arithLogic, "fcmeq"},
    {IC_SIMD_sca_arithLogic, "fcmge"},
    {IC_SIMD_sca_arithLogic, "fcmgt"},
    {IC_SIMD_sca_arithLogic, "fcmle"},
    {IC_SIMD_sca_arithLogic, "fcmlt"},
    /* { IC_SMID_sca_arithLogic , "fmulx"     }, */
    {IC_SIMD_sca_arithLogic, "frecps"},
    {IC_SIMD_sca_arithLogic, "frsqrts"},
    {IC_SIMD_sca_arithLogic, "sqadd"},
    /* { IC_SMID_sca_arithLogic , "sqdmulh"   }, */
    /* { IC_SMID_sca_arithLogic , "sqrdmulh"  }, */
    {IC_SIMD_sca_arithLogic, "sqrshl"},
    /* { IC_SMID_sca_arithLogic , "sqshl"     }, */
    {IC_SIMD_sca_arithLogic, "sqsub"},
    {IC_SIMD_sca_arithLogic, "srshl"},
    {IC_SIMD_sca_arithLogic, "sshl"},
    /* { IC_SMID_sca_arithLogic , "sub"       }, */
    {IC_SIMD_sca_arithLogic, "uqadd"},
    {IC_SIMD_sca_arithLogic, "uqrshl"},
    /* { IC_SMID_sca_arithLogic , "uqshl"     }, */
    {IC_SIMD_sca_arithLogic, "uqsub"},
    {IC_SIMD_sca_arithLogic, "urshl"},
    {IC_SIMD_sca_arithLogic, "ushl"},

    /* { IC_SMID_               , "5.7.6"     }, */
    {IC_SIMD_vec_arithLogic, "addhn"},
    {IC_SIMD_vec_arithLogic, "addhn2"},
    /* { IC_SMID_vec_arithLogic , "pmull"     }, */
    /* { IC_SMID_vec_arithLogic , "pmull2"    }, */
    {IC_SIMD_vec_arithLogic, "raddhn"},
    {IC_SIMD_vec_arithLogic, "raddhn2"},
    {IC_SIMD_vec_arithLogic, "rsubhn"},
    {IC_SIMD_vec_arithLogic, "rsubhn2"},
    {IC_SIMD_vec_arithLogic, "sabal"},
    {IC_SIMD_vec_arithLogic, "sabal2"},
    {IC_SIMD_vec_arithLogic, "sabdl"},
    {IC_SIMD_vec_arithLogic, "sabdl2"},
    {IC_SIMD_vec_arithLogic, "saddl"},
    {IC_SIMD_vec_arithLogic, "saddl2"},
    {IC_SIMD_vec_arithLogic, "saddw"},
    {IC_SIMD_vec_arithLogic, "saddw2"},
    /* { IC_SMID_vec_arithLogic , "smlal"     }, */
    /* { IC_SMID_vec_arithLogic , "smlal2"    }, */
    /* { IC_SMID_vec_arithLogic , "smlsl"     }, */
    /* { IC_SMID_vec_arithLogic , "smlsl2"    }, */
    /* { IC_SMID_vec_arithLogic , "smull"     }, */
    /* { IC_SMID_vec_arithLogic , "smull2"    }, */
    /* { IC_SMID_vec_arithLogic , "sqdmlal"   }, */
    /* { IC_SMID_vec_arithLogic , "sqdmlal2"  }, */
    /* { IC_SMID_vec_arithLogic , "sqdmlsl"   }, */
    /* { IC_SMID_vec_arithLogic , "sqdmlsl2"  }, */
    /* { IC_SMID_vec_arithLogic , "sqdmull"   }, */
    /* { IC_SMID_vec_arithLogic , "sqdmull2"  }, */
    {IC_SIMD_vec_arithLogic, "ssubl"},
    {IC_SIMD_vec_arithLogic, "ssubl2"},
    {IC_SIMD_vec_arithLogic, "ssubw"},
    {IC_SIMD_vec_arithLogic, "ssubw2"},
    {IC_SIMD_vec_arithLogic, "subhn"},
    {IC_SIMD_vec_arithLogic, "subhn2"},
    {IC_SIMD_vec_arithLogic, "uabal"},
    {IC_SIMD_vec_arithLogic, "uabal2"},
    {IC_SIMD_vec_arithLogic, "uabdl"},
    {IC_SIMD_vec_arithLogic, "uabdl2"},
    {IC_SIMD_vec_arithLogic, "uaddl"},
    {IC_SIMD_vec_arithLogic, "uaddl2"},
    {IC_SIMD_vec_arithLogic, "uaddw"},
    {IC_SIMD_vec_arithLogic, "uaddw2"},
    {IC_SIMD_vec_arithLogic, "umlal"},
    {IC_SIMD_vec_arithLogic, "umlal2"},
    {IC_SIMD_vec_arithLogic, "umlsl"},
    {IC_SIMD_vec_arithLogic, "umlsl2"},
    /* { IC_SMID_vec_arithLogic , "umull"     }, */
    {IC_SIMD_vec_arithLogic, "usubl"},
    {IC_SIMD_vec_arithLogic, "usubl2"},
    {IC_SIMD_vec_arithLogic, "usubw"},
    {IC_SIMD_vec_arithLogic, "usubw2"},

    /* { IC_SMID_               , "5.7.7"     }, */
    /* { IC_SMID_sca_arithLogic , "sqdmlal"   }, */
    /* { IC_SMID_sca_arithLogic , "sqdmlsl"   }, */
    /* { IC_SMID_sca_arithLogic , "sqdmull"   }, */

    /* { IC_SIMD_               , "5.7.8"     }, */
    /* { IC_SIMD_vec_arithLogic , "abs"       }, */
    /* { IC_SIMD_vec_arithLogic , "cls"       }, */
    /* { IC_SIMD_vec_arithLogic , "clz"       }, */
    {IC_SIMD_vec_arithLogic, "cnt"},
    /* { IC_SIMD_vec_arithLogic , "fabs"      }, */
    {IC_SIMD_vec_arithLogic, "fcvtl"},
    {IC_SIMD_vec_arithLogic, "fcvtl2"},
    {IC_SIMD_vec_arithLogic, "fcvtn"},
    {IC_SIMD_vec_arithLogic, "fcvtn2"},
    /* { IC_SIMD_vec_arithLogic , "fcvtxn"    }, */
    {IC_SIMD_vec_arithLogic, "fcvtxn2"},
    /* { IC_SIMD_vec_arithLogic , "fneg"      }, */
    /* { IC_SIMD_vec_arithLogic , "frecpe"    }, */
    /* { IC_SIMD_vec_arithLogic , "frintx"    }, */
    /* { IC_SIMD_vec_arithLogic , "frsqrte"   }, */
    /* { IC_SIMD_vec_arithLogic , "fsqrt"     }, */
    /* { IC_SIMD_vec_arithLogic , "mvn"       }, */
    /* { IC_SIMD_vec_arithLogic , "neg"       }, */
    {IC_SIMD_vec_arithLogic, "not"},
    /* { IC_SIMD_vec_arithLogic , "rbit"      }, */
    /* { IC_SIMD_vec_arithLogic , "rev16"     }, */
    /* { IC_SIMD_vec_arithLogic , "rev32"     }, */
    {IC_SIMD_vec_arithLogic, "rev64"},
    {IC_SIMD_vec_arithLogic, "sadalp"},
    {IC_SIMD_vec_arithLogic, "saddlp"},
    /* { IC_SIMD_vec_arithLogic , "sqabs"     }, */
    /* { IC_SIMD_vec_arithLogic , "sqneg"     }, */
    /* { IC_SIMD_vec_arithLogic , "sqxtn"     }, */
    {IC_SIMD_vec_arithLogic, "sqxtn2"},
    /* { IC_SIMD_vec_arithLogic , "sqxtun"    }, */
    {IC_SIMD_vec_arithLogic, "sqxtun2"},
    /* { IC_SIMD_vec_arithLogic , "suqadd"    }, */
    {IC_SIMD_vec_arithLogic, "uadalp"},
    {IC_SIMD_vec_arithLogic, "uaddlp"},
    /* { IC_SIMD_vec_arithLogic , "uqxtn"     }, */
    {IC_SIMD_vec_arithLogic, "uqxtn2"},
    {IC_SIMD_vec_arithLogic, "urecpe"},
    {IC_SIMD_vec_arithLogic, "ursqrte"},
    /* { IC_SIMD_vec_arithLogic , "usqadd"    }, */
    {IC_SIMD_vec_arithLogic, "xtn"},
    {IC_SIMD_vec_arithLogic, "xtn2"},

    /* { IC_SIMD_               , "5.7.9"     }, */
    {IC_SIMD_sca_arithLogic, "abs"},
    {IC_SIMD_sca_arithLogic, "fcvtxn"},
    {IC_SIMD_sca_arithLogic, "frecpe"},
    {IC_SIMD_sca_arithLogic, "frecpx"},
    {IC_SIMD_sca_arithLogic, "frsqrte"},
    /* { IC_SIMD_sca_arithLogic , "neg"       }, */
    {IC_SIMD_sca_arithLogic, "sqabs"},
    {IC_SIMD_sca_arithLogic, "sqneg"},
    {IC_SIMD_sca_arithLogic, "sqxtn"},
    {IC_SIMD_sca_arithLogic, "sqxtun"},
    {IC_SIMD_sca_arithLogic, "suqadd"},
    {IC_SIMD_sca_arithLogic, "uqxtn"},
    {IC_SIMD_sca_arithLogic, "usqadd"},

    /* { IC_SIMD_               , "5.7.10"    }, */
    /* { IC_SIMD_vec_mult       , "fmla"      }, */
    /* { IC_SIMD_vec_mult       , "fmls"      }, */
    /* { IC_SIMD_vec_mult       , "fmul"      }, */
    /* { IC_SIMD_vec_mult       , "fmulx"     }, */
    {IC_SIMD_vec_mult, "mla"},
    {IC_SIMD_vec_mult, "mls"},
    /* { IC_SIMD_vec_mult       , "mul"       }, */
    {IC_SIMD_vec_mult, "smlal"},
    {IC_SIMD_vec_mult, "smlal2"},
    {IC_SIMD_vec_mult, "smlsl"},
    {IC_SIMD_vec_mult, "smlsl2"},
    /* { IC_SIMD_vec_mult       , "smull"     }, */
    {IC_SIMD_vec_mult, "smull2"},
    /* { IC_SIMD_vec_mult       , "sqdmlal"   }, */
    {IC_SIMD_vec_mult, "sqdmlal2"},
    /* { IC_SIMD_vec_mult       , "sqdmlsl"   }, */
    {IC_SIMD_vec_mult, "sqdmlsl2"},
    /* { IC_SIMD_vec_mult       , "sqdmulh"   }, */
    /* { IC_SIMD_vec_mult       , "sqdmull"   }, */
    {IC_SIMD_vec_mult, "sqdmull2"},
    /* { IC_SIMD_vec_mult       , "sqrdmulh"  }, */
    /* { IC_SIMD_vec_mult       , "umlal"     }, */
    /* { IC_SIMD_vec_mult       , "umlal2"    }, */
    /* { IC_SIMD_vec_mult       , "umlsl"     }, */
    /* { IC_SIMD_vec_mult       , "umlsl2"    }, */
    /* { IC_SIMD_vec_mult       , "umull"     }, */
    {IC_SIMD_vec_mult, "umull2"},

    /* { IC_SIMD_               , "5.7.11"    }, */
    {IC_SIMD_sca_mult, "fmla"},
    {IC_SIMD_sca_mult, "fmls"},
    /* { IC_SIMD_sca_mult       , "fmul"      }, */
    {IC_SIMD_sca_mult, "fmulx"},
    {IC_SIMD_sca_mult, "sqdmlal"},
    {IC_SIMD_sca_mult, "sqdmlsl"},
    {IC_SIMD_sca_mult, "sqdmulh"},
    {IC_SIMD_sca_mult, "sqdmull"},
    {IC_SIMD_sca_mult, "sqrdmulh"},

    /* { IC_SIMD_               , "5.7.12"    }, */
    {IC_SIMD_vec_arithLogic, "ext"},
    {IC_SIMD_vec_arithLogic, "trn1"},
    {IC_SIMD_vec_arithLogic, "trn2"},
    {IC_SIMD_vec_arithLogic, "uzp1"},
    {IC_SIMD_vec_arithLogic, "uzp2"},
    {IC_SIMD_vec_arithLogic, "zip1"},
    {IC_SIMD_vec_arithLogic, "zip2"},

    /* { IC_SIMD_               , "5.7.13"    }, */
    /* { IC_SIMD_vec_arithLogic , "bic"       }, */
    /* { IC_SIMD_vec_arithLogic , "fmov"      }, */
    /* { IC_SIMD_vec_arithLogic , "movi"      }, */
    {IC_SIMD_vec_arithLogic, "mvni"},
    /* { IC_SIMD_vec_arithLogic , "orr"       }, */

    /* { IC_SIMD_               , "5.7.14"    }, */
    {IC_SIMD_vec_arithLogic, "rshrn"},
    {IC_SIMD_vec_arithLogic, "rshrn2"},
    /* { IC_SIMD_vec_arithLogic , "shl"       }, */
    {IC_SIMD_vec_arithLogic, "shrn"},
    {IC_SIMD_vec_arithLogic, "shrn2"},
    /* { IC_SIMD_vec_arithLogic , "sli"       }, */
    /* { IC_SIMD_vec_arithLogic , "sqrshrn"   }, */
    {IC_SIMD_vec_arithLogic, "sqrshrn2"},
    /* { IC_SIMD_vec_arithLogic , "sqrshrun"  }, */
    {IC_SIMD_vec_arithLogic, "sqrshrun2"},
    /* { IC_SIMD_vec_arithLogic , "sqshl"     }, */
    /* { IC_SIMD_vec_arithLogic , "sqshlu"    }, */
    /* { IC_SIMD_vec_arithLogic , "sqshrn"    }, */
    {IC_SIMD_vec_arithLogic, "sqshrn2"},
    /* { IC_SIMD_vec_arithLogic , "sqshrun"   }, */
    {IC_SIMD_vec_arithLogic, "sqshrun2"},
    /* { IC_SIMD_vec_arithLogic , "sri"       }, */
    /* { IC_SIMD_vec_arithLogic , "srshr"     }, */
    /* { IC_SIMD_vec_arithLogic , "srsra"     }, */
    /* { IC_SIMD_vec_arithLogic , "sshll"     }, */
    {IC_SIMD_vec_arithLogic, "sshll2"},
    /* { IC_SIMD_vec_arithLogic , "sshr"      }, */
    /* { IC_SIMD_vec_arithLogic , "ssra"      }, */
    {IC_SIMD_vec_arithLogic, "sxtl"},
    {IC_SIMD_vec_arithLogic, "sxtl2"},
    /* { IC_SIMD_vec_arithLogic , "uqrshrn"   }, */
    {IC_SIMD_vec_arithLogic, "uqrshrn2"},
    /* { IC_SIMD_vec_arithLogic , "uqshl"     }, */
    /* { IC_SIMD_vec_arithLogic , "uqshrn"    }, */
    {IC_SIMD_vec_arithLogic, "uqshrn2"},
    /* { IC_SIMD_vec_arithLogic , "urshr"     }, */
    /* { IC_SIMD_vec_arithLogic , "ursra"     }, */
    {IC_SIMD_vec_arithLogic, "ushll"},
    {IC_SIMD_vec_arithLogic, "ushll2"},
    /* { IC_SIMD_vec_arithLogic , "ushr"      }, */
    /* { IC_SIMD_vec_arithLogic , "usra"      }, */
    {IC_SIMD_vec_arithLogic, "uxtl"},
    {IC_SIMD_vec_arithLogic, "uxtl2"},

    /* { IC_SIMD_               , "5.7.15"    }, */
    {IC_SIMD_sca_arithLogic, "shl"},
    {IC_SIMD_sca_arithLogic, "sli"},
    {IC_SIMD_sca_arithLogic, "sqrshrn"},
    {IC_SIMD_sca_arithLogic, "sqrshrun"},
    {IC_SIMD_sca_arithLogic, "sqshl"},
    {IC_SIMD_sca_arithLogic, "sqshlu"},
    {IC_SIMD_sca_arithLogic, "sqshrn"},
    {IC_SIMD_sca_arithLogic, "sqshrun"},
    {IC_SIMD_sca_arithLogic, "sri"},
    {IC_SIMD_sca_arithLogic, "srshr"},
    {IC_SIMD_sca_arithLogic, "srsra"},
    {IC_SIMD_sca_arithLogic, "sshr"},
    {IC_SIMD_sca_arithLogic, "ssra"},
    {IC_SIMD_sca_arithLogic, "uqrshrn"},
    {IC_SIMD_sca_arithLogic, "uqshl"},
    {IC_SIMD_sca_arithLogic, "uqshrn"},
    {IC_SIMD_sca_arithLogic, "urshr"},
    {IC_SIMD_sca_arithLogic, "ursra"},
    {IC_SIMD_sca_arithLogic, "ushr"},
    {IC_SIMD_sca_arithLogic, "usra"},

    /* { IC_SIMD_               , "5.7.16"    }, */
    /* { IC_SIMD_vec_fp         , "fcvtzs"    }, */
    /* { IC_SIMD_vec_fp         , "fcvtzu"    }, */
    /* { IC_SIMD_vec_fp         , "fcvtxs"    }, */
    /* { IC_SIMD_vec_fp         , "fcvtxu"    }, */
    /* { IC_SIMD_vec_fp         , "scvtf"     }, */
    /* { IC_SIMD_vec_fp         , "ucvtf"     }, */

    /* { IC_SIMD_               , "5.7.17"    }, */
    /* { IC_SIMD_sca_fp         , "fcvtzs"    }, */
    /* { IC_SIMD_sca_fp         , "fcvtzu"    }, */
    {IC_SIMD_sca_fp, "fcvtxs"},
    {IC_SIMD_sca_fp, "fcvtxu"},
    /* { IC_SIMD_sca_fp         , "scvtf"     }, */
    /* { IC_SIMD_sca_fp         , "ucvtf"     }, */

    /* { IC_SIMD_               , "5.7.18"    }, */
    {IC_SIMD_vec_arithLogic, "addv"},
    {IC_SIMD_vec_arithLogic, "fmaxnmv"},
    {IC_SIMD_vec_arithLogic, "fmaxv"},
    {IC_SIMD_vec_arithLogic, "fminnmv"},
    {IC_SIMD_vec_arithLogic, "fminv"},
    {IC_SIMD_vec_arithLogic, "saddlv"},
    {IC_SIMD_vec_arithLogic, "smaxv"},
    {IC_SIMD_vec_arithLogic, "sminv"},
    {IC_SIMD_vec_arithLogic, "uaddlv"},
    {IC_SIMD_vec_arithLogic, "umaxv"},
    {IC_SIMD_vec_arithLogic, "uminv"},

    /* { IC_SIMD_               , "5.7.19"    }, */
    /* { IC_SIMD_vec_arithLogic , "addp"      }, */
    /* { IC_SIMD_vec_arithLogic , "faddp"     }, */
    /* { IC_SIMD_vec_arithLogic , "fmaxnmp"   }, */
    /* { IC_SIMD_vec_arithLogic , "fmaxp"     }, */
    /* { IC_SIMD_vec_arithLogic , "fminnmp"   }, */
    /* { IC_SIMD_vec_arithLogic , "fminp"     }, */
    {IC_SIMD_vec_arithLogic, "smaxp"},
    {IC_SIMD_vec_arithLogic, "sminp"},
    {IC_SIMD_vec_arithLogic, "umaxp"},
    {IC_SIMD_vec_arithLogic, "uminp"},

    /* { IC_SIMD_               , "5.7.20"    }, */
    {IC_SIMD_sca_arithLogic, "addp"},
    {IC_SIMD_sca_arithLogic, "faddp"},
    {IC_SIMD_sca_arithLogic, "fmaxp"},
    {IC_SIMD_sca_arithLogic, "fmaxnmp"},
    {IC_SIMD_sca_arithLogic, "fminp"},
    {IC_SIMD_sca_arithLogic, "fminnmp"},

    /* { IC_SIMD_               , "5.7.21"    }, */
    {IC_SIMD_vec_tbl, "tbl"},
    {IC_SIMD_vec_tbl, "tbx"},

    /* { IC_SIMD_               , "5.7.22"    }, */
    /* { IC_SIMD_               , "5.7.22.1"  }, */
    {IC_SIMD_vec_memoryLoad, "ld1"},
    {IC_SIMD_vec_memoryLoad, "ld1r"},
    {IC_SIMD_vec_memoryLoad, "ld2"},
    {IC_SIMD_vec_memoryLoad, "ld2r"},
    {IC_SIMD_vec_memoryLoad, "ld3"},
    {IC_SIMD_vec_memoryLoad, "ld3r"},
    {IC_SIMD_vec_memoryLoad, "ld4"},
    {IC_SIMD_vec_memoryLoad, "ld4r"},
    {IC_SIMD_vec_memoryStore, "st1"},
    {IC_SIMD_vec_memoryStore, "st2"},
    {IC_SIMD_vec_memoryStore, "st3"},
    {IC_SIMD_vec_memoryStore, "st4"},

    /* { IC_SIMD_               , "5.7.23"    }, */
    /* { IC_SIMD_               , "5.7.24"    }, */
    {IC_SIMD_crypto, "aesd"},
    {IC_SIMD_crypto, "aese"},
    {IC_SIMD_crypto, "aesimc"},
    {IC_SIMD_crypto, "aesmc"},
    {IC_SIMD_crypto, "pmull"},
    {IC_SIMD_crypto, "pmull2"},
    {IC_SIMD_crypto, "sha1c"},
    {IC_SIMD_crypto, "sha1h"},
    {IC_SIMD_crypto, "sha1m"},
    {IC_SIMD_crypto, "sha1p"},
    {IC_SIMD_crypto, "sha1su0"},
    {IC_SIMD_crypto, "sha1su1"},
    {IC_SIMD_crypto, "sha256h"},
    {IC_SIMD_crypto, "sha256h2"},
    {IC_SIMD_crypto, "sha256su0"},
    {IC_SIMD_crypto, "sha256su1"},

    /* { IC_system              , "5.8.1"     }, */
    /* { IC_system              , "5.8.1.1"   }, */
    {IC_system, "hvc"},
    {IC_system, "smc"},
    {IC_system, "svc"},

    /* { IC_system              , "5.8.1.2"   }, */
    {IC_system, "brk"},
    {IC_system, "dcps1"},
    {IC_system, "dcps2"},
    {IC_system, "dcps3"},
    {IC_system, "drps"},
    {IC_system, "hlt"},

    /* { IC_system              , "5.8.2"     }, */
    {IC_system, "mrs"},
    {IC_system, "msr"},

    /* { IC_system              , "5.8.3"     }, */
    {IC_system, "sys"},
    {IC_system, "sysl"},
    {IC_system, "ic"},
    {IC_system, "dc"},

    /* { IC_system              , "5.8.4"     }, */
    {IC_system, "nop"},
    {IC_system, "yield"},
    {IC_system, "wfe"},
    {IC_system, "wfi"},
    {IC_system, "sev"},
    {IC_system, "sevl"},

    /* { IC_system              , "5.8.5"     }, */
    {IC_system, "clrex"},
    {IC_system, "dsb"},
    {IC_system, "dmb"},
    {IC_system, "isb"},
    {IC_system, "tlbi"},

    {IC_system, "eret"},

    {IC_default, NULL}};

// Root of tree maintained by the treeSarch routines
static void *instructionTableSearchTreeRoot = NULL;

//
// Comparison function used by treeSearch routines
//
static int
instructionTableCompare(const void *pa, const void *pb) {
    instructionTableP a = ((instructionTableP)pa);
    instructionTableP b = ((instructionTableP)pb);

    return strcmp(a->mnemonic, b->mnemonic);
}

//
// Find instruction table entry by mnemonic
//
static instructionTableP
instructionTableLookup(void **rootP, const char *mnemonic) {
    instructionTablePP node;
    instructionTableT searchNode = {.mnemonic = mnemonic};

    node = treeFind((void *)&searchNode, rootP, instructionTableCompare);

    if (node) {
        VMI_ASSERT(strcmp((*node)->mnemonic, mnemonic) == 0,
                "node mismatch");
    }

    return node ? *node : NULL;
}

//
// Add an instruction table entry to the treeSearch tree
//
static void
instructionTableAdd(void **rootP, instructionTableP entry) {
    instructionTablePP node =
        treeSearch((void *)entry, rootP, instructionTableCompare);

    VMI_ASSERT(*node == entry,
            "Multiple instructionTable entries with mnemonic '%s'",
            entry->mnemonic);
}

//
// Return string corresponding to instruction class
//
static const char *instrClassName(instrClassesE class) {
#define CASE(_C)            \
    case _C:            \
                        return #_C; \
    break
    switch (class) {
        CASE(IC_default);
        CASE(IC_initial);
        CASE(IC_branch);
        CASE(IC_memory);
        CASE(IC_memoryLoad);
        CASE(IC_memoryStore);
        CASE(IC_arithLogic);
        CASE(IC_arithLogicMov);
        CASE(IC_multDiv);
        CASE(IC_fp);
        CASE(IC_fpMov);
        CASE(IC_SIMD_arithLogic);
        CASE(IC_SIMD_arithLogicMov);
        CASE(IC_SIMD_vec_arithLogic);
        CASE(IC_SIMD_sca_arithLogic);
        CASE(IC_SIMD_vec_mult);
        CASE(IC_SIMD_sca_mult);
        CASE(IC_SIMD_sca_fp);
        CASE(IC_SIMD_vec_tbl);
        CASE(IC_SIMD_vec_memoryLoad);
        CASE(IC_SIMD_vec_memoryStore);
        CASE(IC_SIMD_crypto);
        CASE(IC_system);

        default:
        break;
    }
#undef CASE

    return "???";
}

void
armReport(vmiosObjectP object) {
    const char *ceFile = object->filename;

    vmiPrintf(
            "\nEstimated Default instructions                                  " FMT_64u
            "\nEstimated branch instructions                                   " FMT_64u
            "\nEstimated memory instructions                                   " FMT_64u
            "\nEstimated memory LOAD instructions                              " FMT_64u
            "\nEstimated memory STORE instructions                             " FMT_64u
            "\nEstimated Arithmetic and Logic instructions                     " FMT_64u
            "\nEstimated Arithmetic and Logic MOV instructions                 " FMT_64u
            "\nEstimated Mult and Div instructions                             " FMT_64u
            "\nEstimated FP instructions                                       " FMT_64u
            "\nEstimated FP MOV instructions                                   " FMT_64u
            "\nEstimated SIMD Arithmetic and Logic instructions                " FMT_64u
            "\nEstimated SIMD Arithmetic and Logic MOV instructions            " FMT_64u
            "\nEstimated SIMD Vector Arithmetic and Logic instructions         " FMT_64u
            "\nEstimated SIMD Scalar Arithmetic and Logic instructions         " FMT_64u
            "\nEstimated SIMD Vector Mult instructions                         " FMT_64u
            "\nEstimated SIMD Scalar Multi instructions                        " FMT_64u
            "\nEstimated SIMD Scalar FP instructions                           " FMT_64u
            "\nEstimated SIMD Vector Look up Table instructions                " FMT_64u
            "\nEstimated SIMD Vector memory LOAD instructions                  " FMT_64u
            "\nEstimated SIMD Vector memory STORE instructions                 " FMT_64u
            "\nEstimated SIMD Crypto instructions                              " FMT_64u
            "\nEstimated System instructions                                   " FMT_64u
            "\nSum:          						     " FMT_64u,
        object->defaultCount,
        object->branchCount,
        object->memoryCount,
        object->memoryLoadCount,
        object->memoryStoreCount,
        object->arithLogicCount,
        object->arithLogicMovCount,
        object->multDivCount,
        object->fpCount,
        object->fpMovCount,
        object->SIMDarithLogicCount,
        object->SIMDarithLogicMovCount,
        object->SIMDvec_arithLogicCount,
        object->SIMDsca_arithLogicCount,
        object->SIMDvec_multCount,
        object->SIMDsca_multCount,
        object->SIMDsca_fpCount,
        object->SIMDvec_tblCount,
        object->SIMDvec_memoryLoadCount,
        object->SIMDvec_memoryStoreCount,
        object->SIMDcryptoCount,
        object->systemCount,

        object->defaultCount+
            object->branchCount+
            object->memoryCount+
            object->memoryLoadCount+
            object->memoryStoreCount+
            object->arithLogicCount+
            object->arithLogicMovCount+
            object->multDivCount+
            object->fpCount+
            object->fpMovCount+
            object->SIMDarithLogicCount+
            object->SIMDarithLogicMovCount+
            object->SIMDvec_arithLogicCount+
            object->SIMDsca_arithLogicCount+
            object->SIMDvec_multCount+
            object->SIMDsca_multCount+
            object->SIMDsca_fpCount+
            object->SIMDvec_tblCount+
            object->SIMDvec_memoryLoadCount+
            object->SIMDvec_memoryStoreCount+
            object->SIMDcryptoCount+
            object->systemCount);


    vmiPrintf(
            "\n\n\nInteger Registers\n\n\nx0        " FMT_64u
            "\nx1        " FMT_64u
            "\nx2        " FMT_64u
            "\nx3        " FMT_64u
            "\nx4        " FMT_64u
            "\nx5        " FMT_64u
            "\nx6        " FMT_64u
            "\nx7        " FMT_64u
            "\nx8        " FMT_64u
            "\nx9        " FMT_64u
            "\nx10       " FMT_64u
            "\nx11       " FMT_64u
            "\nx12       " FMT_64u
            "\nx13       " FMT_64u
            "\nx14       " FMT_64u
            "\nx15       " FMT_64u
            "\nx16       " FMT_64u
            "\nx17       " FMT_64u
            "\nx18       " FMT_64u
            "\nx19       " FMT_64u
            "\nx20       " FMT_64u
            "\nx21       " FMT_64u
            "\nx22       " FMT_64u
            "\nx23       " FMT_64u
            "\nx24       " FMT_64u
            "\nx25       " FMT_64u
            "\nx26       " FMT_64u
            "\nx27       " FMT_64u
            "\nx28       " FMT_64u
            "\nx29       " FMT_64u
            "\nx30       " FMT_64u,
        object->regX0,
        object->regX1,
        object->regX2,
        object->regX3,
        object->regX4,
        object->regX5,
        object->regX6,
        object->regX7,
        object->regX8,
        object->regX9,
        object->regX10,
        object->regX11,
        object->regX12,
        object->regX13,
        object->regX14,
        object->regX15,
        object->regX16,
        object->regX17,
        object->regX18,
        object->regX19,
        object->regX20,
        object->regX21,
        object->regX22,
        object->regX23,
        object->regX24,
        object->regX25,
        object->regX26,
        object->regX27,
        object->regX28,
        object->regX29,
        object->regX30);

    vmiPrintf(
            "\n\n\nFloating Point Registers\n\n\nv0        " FMT_64u
            "\nv1        " FMT_64u
            "\nv2        " FMT_64u
            "\nv3        " FMT_64u
            "\nv4        " FMT_64u
            "\nv5        " FMT_64u
            "\nv6        " FMT_64u
            "\nv7        " FMT_64u
            "\nv8        " FMT_64u
            "\nv9        " FMT_64u
            "\nv10       " FMT_64u
            "\nv11       " FMT_64u
            "\nv12       " FMT_64u
            "\nv13       " FMT_64u
            "\nv14       " FMT_64u
            "\nv15       " FMT_64u
            "\nv16       " FMT_64u
            "\nv17       " FMT_64u
            "\nv18       " FMT_64u
            "\nv19       " FMT_64u
            "\nv20       " FMT_64u
            "\nv21       " FMT_64u
            "\nv22       " FMT_64u
            "\nv23       " FMT_64u
            "\nv24       " FMT_64u
            "\nv25       " FMT_64u
            "\nv26       " FMT_64u
            "\nv27       " FMT_64u
            "\nv28       " FMT_64u
            "\nv29       " FMT_64u
            "\nv30       " FMT_64u
            "\nv31       " FMT_64u,
        object->regD0,
        object->regD1,
        object->regD2,
        object->regD3,
        object->regD4,
        object->regD5,
        object->regD6,
        object->regD7,
        object->regD8,
        object->regD9,
        object->regD10,
        object->regD11,
        object->regD12,
        object->regD13,
        object->regD14,
        object->regD15,
        object->regD16,
        object->regD17,
        object->regD18,
        object->regD19,
        object->regD20,
        object->regD21,
        object->regD22,
        object->regD23,
        object->regD24,
        object->regD25,
        object->regD26,
        object->regD27,
        object->regD28,
        object->regD29,
        object->regD30,
        object->regD31);


    vmiPrintf(
            "\n\n\nOthers"
            "\n\n\nsp                  " FMT_64u
            "\npc                  " FMT_64u
            "\nOther               " FMT_64u
            "\nFault Number       " FMT_64u
            "\niCount             " FMT_64u "\n\n",
            object->regSp,
            object->regPc,
            object->regOthers,
            object->faultIcount,
            object->iCount);

    if (ceFile) {
        FILE *pFile = fopen(ceFile, "a");

        if (pFile) {

            fprintf(pFile,
                    "\nEstimated Default instructions                                  " FMT_64u
                    "\nEstimated branch instructions                                   " FMT_64u
                    "\nEstimated memory instructions                                   " FMT_64u
                    "\nEstimated memory LOAD instructions                              " FMT_64u
                    "\nEstimated memory STORE instructions                             " FMT_64u
                    "\nEstimated Arithmetic and Logic instructions                     " FMT_64u
                    "\nEstimated Arithmetic and Logic MOV instructions                 " FMT_64u
                    "\nEstimated Mult and Div instructions                             " FMT_64u
                    "\nEstimated FP instructions                                       " FMT_64u
                    "\nEstimated FP MOV instructions                                   " FMT_64u
                    "\nEstimated SIMD Arithmetic and Logic instructions                " FMT_64u
                    "\nEstimated SIMD Arithmetic and Logic MOV instructions            " FMT_64u
                    "\nEstimated SIMD Vector Arithmetic and Logic instructions         " FMT_64u
                    "\nEstimated SIMD Scalar Arithmetic and Logic instructions         " FMT_64u
                    "\nEstimated SIMD Vector Mult instructions                         " FMT_64u
                    "\nEstimated SIMD Scalar Multi instructions                        " FMT_64u
                    "\nEstimated SIMD Scalar FP instructions                           " FMT_64u
                    "\nEstimated SIMD Vector Look up Table instructions                " FMT_64u
                    "\nEstimated SIMD Vector memory LOAD instructions                  " FMT_64u
                    "\nEstimated SIMD Vector memory STORE instructions                 " FMT_64u
                    "\nEstimated SIMD Crypto instructions                              " FMT_64u
                    "\nEstimated System instructions                                   " FMT_64u
                    "\nSum:          						     " FMT_64u,
                object->defaultCount,
                object->branchCount,
                object->memoryCount,
                object->memoryLoadCount,
                object->memoryStoreCount,
                object->arithLogicCount,
                object->arithLogicMovCount,
                object->multDivCount,
                object->fpCount,
                object->fpMovCount,
                object->SIMDarithLogicCount,
                object->SIMDarithLogicMovCount,
                object->SIMDvec_arithLogicCount,
                object->SIMDsca_arithLogicCount,
                object->SIMDvec_multCount,
                object->SIMDsca_multCount,
                object->SIMDsca_fpCount,
                object->SIMDvec_tblCount,
                object->SIMDvec_memoryLoadCount,
                object->SIMDvec_memoryStoreCount,
                object->SIMDcryptoCount,
                object->systemCount,

                object->defaultCount+
                    object->branchCount+
                    object->memoryCount+
                    object->memoryLoadCount+
                    object->memoryStoreCount+
                    object->arithLogicCount+
                    object->arithLogicMovCount+
                    object->multDivCount+
                    object->fpCount+
                    object->fpMovCount+
                    object->SIMDarithLogicCount+
                    object->SIMDarithLogicMovCount+
                    object->SIMDvec_arithLogicCount+
                    object->SIMDsca_arithLogicCount+
                    object->SIMDvec_multCount+
                    object->SIMDsca_multCount+
                    object->SIMDsca_fpCount+
                    object->SIMDvec_tblCount+
                    object->SIMDvec_memoryLoadCount+
                    object->SIMDvec_memoryStoreCount+
                    object->SIMDcryptoCount+
                    object->systemCount);


            fprintf(pFile,
                    "\n\n\nInteger Registers\n\n\nx0        " FMT_64u
                    "\nx1        " FMT_64u
                    "\nx2        " FMT_64u
                    "\nx3        " FMT_64u
                    "\nx4        " FMT_64u
                    "\nx5        " FMT_64u
                    "\nx6        " FMT_64u
                    "\nx7        " FMT_64u
                    "\nx8        " FMT_64u
                    "\nx9        " FMT_64u
                    "\nx10       " FMT_64u
                    "\nx11       " FMT_64u
                    "\nx12       " FMT_64u
                    "\nx13       " FMT_64u
                    "\nx14       " FMT_64u
                    "\nx15       " FMT_64u
                    "\nx16       " FMT_64u
                    "\nx17       " FMT_64u
                    "\nx18       " FMT_64u
                    "\nx19       " FMT_64u
                    "\nx20       " FMT_64u
                    "\nx21       " FMT_64u
                    "\nx22       " FMT_64u
                    "\nx23       " FMT_64u
                    "\nx24       " FMT_64u
                    "\nx25       " FMT_64u
                    "\nx26       " FMT_64u
                    "\nx27       " FMT_64u
                    "\nx28       " FMT_64u
                    "\nx29       " FMT_64u
                    "\nx30       " FMT_64u,
                object->regX0,
                object->regX1,
                object->regX2,
                object->regX3,
                object->regX4,
                object->regX5,
                object->regX6,
                object->regX7,
                object->regX8,
                object->regX9,
                object->regX10,
                object->regX11,
                object->regX12,
                object->regX13,
                object->regX14,
                object->regX15,
                object->regX16,
                object->regX17,
                object->regX18,
                object->regX19,
                object->regX20,
                object->regX21,
                object->regX22,
                object->regX23,
                object->regX24,
                object->regX25,
                object->regX26,
                object->regX27,
                object->regX28,
                object->regX29,
                object->regX30);

            fprintf(pFile,
                    "\n\n\nFloating Point Registers\n\n\nv0        " FMT_64u
                    "\nv1        " FMT_64u
                    "\nv2        " FMT_64u
                    "\nv3        " FMT_64u
                    "\nv4        " FMT_64u
                    "\nv5        " FMT_64u
                    "\nv6        " FMT_64u
                    "\nv7        " FMT_64u
                    "\nv8        " FMT_64u
                    "\nv9        " FMT_64u
                    "\nv10       " FMT_64u
                    "\nv11       " FMT_64u
                    "\nv12       " FMT_64u
                    "\nv13       " FMT_64u
                    "\nv14       " FMT_64u
                    "\nv15       " FMT_64u
                    "\nv16       " FMT_64u
                    "\nv17       " FMT_64u
                    "\nv18       " FMT_64u
                    "\nv19       " FMT_64u
                    "\nv20       " FMT_64u
                    "\nv21       " FMT_64u
                    "\nv22       " FMT_64u
                    "\nv23       " FMT_64u
                    "\nv24       " FMT_64u
                    "\nv25       " FMT_64u
                    "\nv26       " FMT_64u
                    "\nv27       " FMT_64u
                    "\nv28       " FMT_64u
                    "\nv29       " FMT_64u
                    "\nv30       " FMT_64u
                    "\nv31       " FMT_64u,
                object->regD0,
                object->regD1,
                object->regD2,
                object->regD3,
                object->regD4,
                object->regD5,
                object->regD6,
                object->regD7,
                object->regD8,
                object->regD9,
                object->regD10,
                object->regD11,
                object->regD12,
                object->regD13,
                object->regD14,
                object->regD15,
                object->regD16,
                object->regD17,
                object->regD18,
                object->regD19,
                object->regD20,
                object->regD21,
                object->regD22,
                object->regD23,
                object->regD24,
                object->regD25,
                object->regD26,
                object->regD27,
                object->regD28,
                object->regD29,
                object->regD30,
                object->regD31);


            fprintf(pFile,
                    "\n\n\nOthers"
                    "\n\n\nsp                  " FMT_64u
                    "\npc                  " FMT_64u
                    "\nOther               " FMT_64u
                    "\nFault Number       " FMT_64u
                    "\niCount             " FMT_64u "\n\n",
                    object->regSp,
                    object->regPc,
                    object->regOthers,
                    object->faultIcount,
                    object->iCount);
            fclose(pFile);

        } else {
            vmiMessage("W", PREFIX "_WFE",
                    "Not able to open file '%s' to write "
                    "Estimated Cycles",
                    ceFile);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// CONDITIONAL INSTRUCTIONS
////////////////////////////////////////////////////////////////////////////////

//
// Condition code enumeration
//
typedef enum armConditionE {
    ARM_C_EQ,  // ZF==1
    ARM_C_NE,  // ZF==0
    ARM_C_CS,  // CF==1
    ARM_C_CC,  // CF==0
    ARM_C_MI,  // NF==1
    ARM_C_PL,  // NF==0
    ARM_C_VS,  // VF==1
    ARM_C_VC,  // VF==0
    ARM_C_HI,  // (CF==1) && (ZF==0)
    ARM_C_LS,  // (CF==0) || (ZF==1)
    ARM_C_GE,  // NF==VF
    ARM_C_LT,  // NF!=VF
    ARM_C_GT,  // (ZF==0) && (NF==VF)
    ARM_C_LE,  // (ZF==1) || (NF!=VF)
    ARM_C_AL,  // always
} armCondition;

// table mapping from condition name to enumeration
static const char *condCodeNames[] = {[ARM_C_EQ] = "eq", [ARM_C_NE] = "ne",
    [ARM_C_CS] = "cs", [ARM_C_CC] = "cc",
    [ARM_C_MI] = "mi", [ARM_C_PL] = "pl",
    [ARM_C_VS] = "vs", [ARM_C_VC] = "vc",
    [ARM_C_HI] = "hi", [ARM_C_LS] = "ls",
    [ARM_C_GE] = "ge", [ARM_C_LT] = "lt",
    [ARM_C_GT] = "gt", [ARM_C_LE] = "le",
    [ARM_C_AL] = "al"};

#define WIDTH(_W, _ARG) ((_ARG) & ((1 << (_W)) - 1))
#define GETFIELD(_I, _W, _P) (WIDTH(_W, (_I >> _P)))

#define VFLAG 0x01
#define CFLAG 0x02
#define ZFLAG 0x04
#define NFLAG 0x08

//
// This defines a search string and pattern to match a named field in the
// uncooked disassembly string
//
#define FIELD_S(_NAME) \
    " "_NAME,      \
    " "_NAME   \
    "%s"

//
// Return string extracted from disassembly string using the passed pattern
//
static Bool
parseString(const char *disass, const char *key, const char *pattern,
        char *result) {
    if ((disass = strstr(disass, key))) {
        sscanf(disass, pattern, result);
        return True;
    } else {
        return False;
    }
}

//
// Return armCondition extracted from disassembly string
//
static armCondition
parseCond(const char *disass) {
    armCondition result = ARM_C_AL;
    char cond[4];

    // scan forward to optional key
    if (parseString(disass, FIELD_S("COND:"), cond)) {
        // find matching entry in table
        for (result = 0; result < ARM_C_AL; result++) {
            if (!strcmp(cond, condCodeNames[result])) {
                break;
            }
        }
    }

    // return condition
    return result;
}

//
// Get the condition flags which are bits 31:28 of the cpsr
//
static Uns32
armCondFlags(vmiosObjectP object) {
    armCEDataP CEData = object->procCEData;
    Uns32 cpsr;

    vmiosRegRead(object->processor, CEData->cpsr, &cpsr);

    return GETFIELD(cpsr, 4, 28);
}

//
// Return false if the instructions should not be executed according to any
// condition code associated with it (this includes IT blocks)
//
Bool
armCondExecution(vmiosObjectP object, armCondition cond) {
    Bool result = True;

    if (cond != ARM_C_AL) {
        // condition is not 'always', so check condition flags
        Uns32 flags = armCondFlags(object);

        Bool vf = (flags & VFLAG) != 0;
        Bool cf = (flags & CFLAG) != 0;
        Bool zf = (flags & ZFLAG) != 0;
        Bool nf = (flags & NFLAG) != 0;

        switch (cond) {
            case ARM_C_EQ:
                return zf;
            case ARM_C_NE:
                return !zf;
            case ARM_C_CS:
                return cf;
            case ARM_C_CC:
                return !cf;
            case ARM_C_MI:
                return nf;
            case ARM_C_PL:
                return !nf;
            case ARM_C_VS:
                return vf;
            case ARM_C_VC:
                return !vf;
            case ARM_C_GT:
                return !zf && (nf == vf);
            case ARM_C_LE:
                return zf || (nf != vf);
            case ARM_C_GE:
                return nf == vf;
            case ARM_C_LT:
                return nf != vf;
            case ARM_C_HI:
                return cf && !zf;
            case ARM_C_LS:
                return !cf || zf;
            default:
                VMI_ABORT("unimplemented condition");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
// INTERCEPT CALLBACKS
////////////////////////////////////////////////////////////////////////////////
//
// Return string corresponding to vmiBranchReason enums
//
/*static const char *branchReasonString(vmiBranchReason reason) {

  switch (reason) {
  case VMIBR_UNCOND:         return "unconditional branch taken"; break;
  case VMIBR_COND_TAKEN:     return "conditional branch taken"; break;
  case VMIBR_COND_NOT_TAKEN: return "conditional branch not taken"; break;
  case VMIBR_PC_SET:         return "PC set by non instruction route";
  break;
  default: break;
  }

  return "???";

  } */

#define PIPELINE_PENALTY 2
//
// Branch notifier callback
// Add the Pipeline refill cycles (See section 3.3.1 in ARM DDI0439D)
// Note this is listed as 1 to 3 cycles - we just use 2 as an average
//
/*static VMI_BRANCH_REASON_FN(armBranchNotifier) {

  vmiosObjectP object        = userData;
  Uns32        penaltyCycles = 0;

  if (object->enabled) {

  switch(reason) {
  case VMIBR_UNCOND: {
  penaltyCycles = PIPELINE_PENALTY;
  break;
  }
  case VMIBR_COND_TAKEN: {
  penaltyCycles = PIPELINE_PENALTY;
  break;
  }
  case VMIBR_COND_NOT_TAKEN: {
  break;
  }
  case VMIBR_PC_SET: {
  penaltyCycles = PIPELINE_PENALTY;
  break;
  }
  default: {
  VMI_ABORT("Invalid vmiBranchReason %d", reason);
  break;
  }
  }

  object->additionalCycles += penaltyCycles;
  vmiMessage(
  "I", PREFIX "_BAV",
  "%s: PC: 0x"FMT_A08x": %s: New PC: 0x"FMT_Ax": %d penalty cycles added\n",
  vmirtProcessorName(processor),
  prevPC,
  branchReasonString(reason),
  thisPC,
  penaltyCycles
  );
  }
  } */

/* static const Uns32 divisionCycles[] = {   5, 5, 5, 5  //00
   , 6, 6, 6, 6  //04
   , 7, 7, 7, 7  //08
   , 8, 8, 8, 8  //12
   , 9, 9, 9, 9  //16
   ,10,10,10,10  //20
   ,11,11,11,11  //24
   ,12,12,12,12};//28
   */

static void addRegCount(Addr thisPC, vmiosObjectP object, vmiRegInfoCP reg){

    if (object->active){

        if (object->watchReg){
            if (strcmp(reg->name, object->targetRegister) == 0){
                FILE * pfile = fopen(object->filename, "a");
                fprintf(pfile, "First Target Register Use After FI: %s\n", vmirtDisassemble(object->processor, thisPC, DSA_NORMAL));
                fclose(pfile);
                object->watchReg = False;
            }
        }

        if (strcmp(reg->name, "x0") == 0)
            object->regX0++;
        else if (strcmp(reg->name, "x1") == 0)
            object->regX1++;
        else if (strcmp(reg->name, "x2") == 0)
            object->regX2++;
        else if (strcmp(reg->name, "x3") == 0)
            object->regX3++;
        else if (strcmp(reg->name, "x4") == 0)
            object->regX4++;
        else if (strcmp(reg->name, "x5") == 0)
            object->regX5++;
        else if (strcmp(reg->name, "x6") == 0)
            object->regX6++;
        else if (strcmp(reg->name, "x7") == 0)
            object->regX7++;
        else if (strcmp(reg->name, "x8") == 0)
            object->regX8++;
        else if (strcmp(reg->name, "x9") == 0)
            object->regX9++;
        else if (strcmp(reg->name, "x10") == 0)
            object->regX10++;
        else if (strcmp(reg->name, "x11") == 0)
            object->regX11++;
        else if (strcmp(reg->name, "x12") == 0)
            object->regX12++;
        else if (strcmp(reg->name, "x13") == 0)
            object->regX13++;
        else if (strcmp(reg->name, "x14") == 0)
            object->regX14++;
        else if (strcmp(reg->name, "x15") == 0)
            object->regX15++;
        else if (strcmp(reg->name, "x16") == 0)
            object->regX16++;
        else if (strcmp(reg->name, "x17") == 0)
            object->regX17++;
        else if (strcmp(reg->name, "x18") == 0)
            object->regX18++;
        else if (strcmp(reg->name, "x19") == 0)
            object->regX19++;
        else if (strcmp(reg->name, "x20") == 0)
            object->regX20++;
        else if (strcmp(reg->name, "x21") == 0)
            object->regX21++;
        else if (strcmp(reg->name, "x22") == 0)
            object->regX22++;
        else if (strcmp(reg->name, "x23") == 0)
            object->regX23++;
        else if (strcmp(reg->name, "x24") == 0)
            object->regX24++;
        else if (strcmp(reg->name, "x25") == 0)
            object->regX25++;
        else if (strcmp(reg->name, "x26") == 0)
            object->regX26++;
        else if (strcmp(reg->name, "x27") == 0)
            object->regX27++;
        else if (strcmp(reg->name, "x28") == 0)
            object->regX28++;
        else if (strcmp(reg->name, "x29") == 0)
            object->regX29++;
        else if (strcmp(reg->name, "x30") == 0)
            object->regX30++;
        else if (strcmp(reg->name, "sp") == 0)
            object->regSp++;
        else if (strcmp(reg->name, "pc") == 0)
            object->regPc++;
        else if (strcmp(reg->name, "lr") == 0)
            object->regLr++;
        else if (strcmp(reg->name, "d0") == 0 || strcmp(reg->name, "v0") == 0)
            object->regD0++;
        else if (strcmp(reg->name, "d1") == 0 || strcmp(reg->name, "v1") == 0)
            object->regD1++;
        else if (strcmp(reg->name, "d2") == 0 || strcmp(reg->name, "v2") == 0)
            object->regD2++;
        else if (strcmp(reg->name, "d3") == 0 || strcmp(reg->name, "v3") == 0)
            object->regD3++;
        else if (strcmp(reg->name, "d4") == 0 || strcmp(reg->name, "v4") == 0)
            object->regD4++;
        else if (strcmp(reg->name, "d5") == 0 || strcmp(reg->name, "v5") == 0)
            object->regD5++;
        else if (strcmp(reg->name, "d6") == 0 || strcmp(reg->name, "v6") == 0)
            object->regD6++;
        else if (strcmp(reg->name, "d7") == 0 || strcmp(reg->name, "v7") == 0)
            object->regD7++;
        else if (strcmp(reg->name, "d8") == 0 || strcmp(reg->name, "v8") == 0)
            object->regD8++;
        else if (strcmp(reg->name, "d9") == 0 || strcmp(reg->name, "v9") == 0)
            object->regD9++;
        else if (strcmp(reg->name, "d10") == 0 || strcmp(reg->name, "v10") == 0)
            object->regD10++;
        else if (strcmp(reg->name, "d11") == 0 || strcmp(reg->name, "v11") == 0)
            object->regD11++;
        else if (strcmp(reg->name, "d12") == 0 || strcmp(reg->name, "v12") == 0)
            object->regD12++;
        else if (strcmp(reg->name, "d13") == 0 || strcmp(reg->name, "v13") == 0)
            object->regD13++;
        else if (strcmp(reg->name, "d14") == 0 || strcmp(reg->name, "v14") == 0)
            object->regD14++;
        else if (strcmp(reg->name, "d15") == 0 || strcmp(reg->name, "v15") == 0)
            object->regD15++;
        else if (strcmp(reg->name, "d16") == 0 || strcmp(reg->name, "v16") == 0)
            object->regD16++;
        else if (strcmp(reg->name, "d17") == 0 || strcmp(reg->name, "v17") == 0)
            object->regD17++;
        else if (strcmp(reg->name, "d18") == 0 || strcmp(reg->name, "v18") == 0)
            object->regD18++;
        else if (strcmp(reg->name, "d19") == 0 || strcmp(reg->name, "v19") == 0)
            object->regD19++;
        else if (strcmp(reg->name, "d20") == 0 || strcmp(reg->name, "v20") == 0)
            object->regD20++;
        else if (strcmp(reg->name, "d21") == 0 || strcmp(reg->name, "v21") == 0)
            object->regD21++;
        else if (strcmp(reg->name, "d22") == 0 || strcmp(reg->name, "v22") == 0)
            object->regD22++;
        else if (strcmp(reg->name, "d23") == 0 || strcmp(reg->name, "v23") == 0)
            object->regD23++;
        else if (strcmp(reg->name, "d24") == 0 || strcmp(reg->name, "v24") == 0)
            object->regD24++;
        else if (strcmp(reg->name, "d25") == 0 || strcmp(reg->name, "v25") == 0)
            object->regD25++;
        else if (strcmp(reg->name, "d26") == 0 || strcmp(reg->name, "v26") == 0)
            object->regD26++;
        else if (strcmp(reg->name, "d27") == 0 || strcmp(reg->name, "v27") == 0)
            object->regD27++;
        else if (strcmp(reg->name, "d28") == 0 || strcmp(reg->name, "v28") == 0)
            object->regD28++;
        else if (strcmp(reg->name, "d29") == 0 || strcmp(reg->name, "v29") == 0)
            object->regD29++;
        else if (strcmp(reg->name, "d30") == 0 || strcmp(reg->name, "v30") == 0)
            object->regD30++;
        else if (strcmp(reg->name, "d31") == 0 || strcmp(reg->name, "v31") == 0)
            object->regD31++;
        else{
            object->regOthers++;
        }
    }

}

// Emit morph code to add cycles to additionalCycles counter
//
static void
addInstCount(vmiosObjectP object, instrClassesE iClass) {

    if (object->active){

        switch (iClass) {
            case IC_default:
                object->defaultCount++;
                break;

            case IC_branch:
                object->branchCount++;
                break;

            case IC_memory:
                object->memoryCount++;
                break;

            case IC_memoryLoad:
                object->memoryLoadCount++;
                break;

            case IC_memoryStore:
                object->memoryStoreCount++;
                break;

            case IC_arithLogic:
                object->arithLogicCount++;
                break;

            case IC_arithLogicMov:
                object->arithLogicMovCount++;
                break;

            case IC_multDiv:
                object->multDivCount++;
                break;

            case IC_fp:
                object->fpCount++;
                break;

            case IC_fpMov:
                object->fpMovCount++;
                break;

            case IC_SIMD_arithLogic:
                object->SIMDarithLogicCount++;
                break;

            case IC_SIMD_arithLogicMov:
                object->SIMDarithLogicMovCount++;
                break;

            case IC_SIMD_vec_arithLogic:
                object->SIMDvec_arithLogicCount++;
                break;

            case IC_SIMD_sca_arithLogic:
                object->SIMDsca_arithLogicCount++;
                break;

            case IC_SIMD_vec_mult:
                object->SIMDvec_multCount++;
                break;

            case IC_SIMD_sca_mult:
                object->SIMDsca_multCount++;
                break;

            case IC_SIMD_sca_fp:
                object->SIMDsca_fpCount++;
                break;

            case IC_SIMD_vec_tbl:
                object->SIMDvec_tblCount++;
                break;

            case IC_SIMD_vec_memoryLoad:
                object->SIMDvec_memoryLoadCount++;
                break;

            case IC_SIMD_vec_memoryStore:
                object->SIMDvec_memoryStoreCount++;
                break;

            case IC_SIMD_crypto:
                object->SIMDcryptoCount++;
                break;

            case IC_system:
                object->systemCount++;
                break;

            default:
                VMI_ABORT("Invalid instructionCLassE value %d (%s)\n",
                        iClass, instrClassName(iClass));

        }
    }
}

//
// Return the instruction class for the instruction at this address
//
static instrClassesE
getInstructionClass(vmiosObjectP object, Addr thisPC, octiaAttrP attrs,
        armCondition *condCode) {
    instructionTableP entry = NULL;
    const char *disass =
        vmirtDisassemble(object->processor, thisPC, DSA_UNCOOKED);

    // In ARMM Uncooked mode the instruction mnemonic (excluding any
    // conditional execution code) is the first token
    char mnemonic[16];
    if (sscanf(disass, "%15s ", mnemonic) == 1) {
        entry = instructionTableLookup(&instructionTableSearchTreeRoot,
                mnemonic);
    }

    // Look up mnemonic in the instruction class table
    instrClassesE iClass = entry ? entry->instructionClass : IC_default;

    // Parse any conditional execution code on the instruction
    *condCode = parseCond(disass);

    if (DIAG_HIGH) {
        vmiMessage(
                "I", PREFIX "_CLS",
                "%s: Mnemonic: %s, iClass: %s%s%s, Disassembly: " FMT_A08x
                ": '%s'",
                object->name, mnemonic, instrClassName(iClass),
                *condCode == ARM_C_AL ? "" : ", CondCode: ",
                *condCode == ARM_C_AL ? "" : condCodeNames[*condCode],
                thisPC,
                vmirtDisassemble(object->processor, thisPC, DSA_UNCOOKED));
    }

    return iClass;
}

//
// Return true if the instruction uses a constant as an address expression
//
/*static Uns32 hasImmediateOp(octiaAttrP attrs) {

  octiaMemAccessP ma;

  if((ma=ocliaGetFirstMemAccess(attrs))) {
  for(; ma; ma=ocliaGetNextMemAccess(ma)) {

  octiaAddrExpP addrExp = ocliaGetMemAccessAddrExp(ma);

// does it have an immediate operand
if(addrExp->type==OCL_ET_BINARY) {
if(addrExp->b.child[1]->type==OCL_ET_CONST) {
return True;
}
}
}
}

return False;

} */

//
// Return the number of memory accesses (loads/stores) made by the instruction
//
/*static Uns32 accessCount(octiaAttrP attrs) {

  octiaMemAccessP ma;
  Uns32           accesses = 0;

// Acquire the number of access to read and write registers
if((ma=ocliaGetFirstMemAccess(attrs))) {
for(; ma; ma=ocliaGetNextMemAccess(ma)) {

// is it a load or store operation?
octiaMemAccessType maType = ocliaGetMemAccessType(ma);
if (maType == OCL_MAT_LOAD || maType == OCL_MAT_STORE) {
accesses++;
}
}
}

return accesses;

} */

static void addToICount(Addr thisPC, vmiosObjectP object){
    Uns64 c = vmirtGetExecutedICount(object->processor);
    object->iCount = c;

    if (object->active){
        if (c-1 == object->faultIcount){
            object->watchReg = True;
            FILE * pfile = fopen(object->filename, "a");
            fprintf(pfile, "Instruction Executing: %s\n", vmirtDisassemble(object->processor, thisPC, DSA_NORMAL));
            fclose(pfile);
            vmiPrintf("\nIC " FMT_64u, c);
            vmiPrintf("(%s)\n", vmirtDisassemble(object->processor, thisPC, DSA_NORMAL));
        }
        if (object->watchReg){
            vmiPrintf("\nIC " FMT_64u, c);
            vmiPrintf("(%s)\n", vmirtDisassemble(object->processor, thisPC, DSA_NORMAL));
        }
    }

}

#define SELECT_ATTRS                                                     \
    (OCL_DS_REG_R | OCL_DS_REG_W | OCL_DS_RANGE_R | OCL_DS_RANGE_W | \
     OCL_DS_FETCH | OCL_DS_NEXTPC | OCL_DS_ADDRESS)

VMIOS_MORPH_FN(armCEMorphCB) {

    armCEDataP CEData = object->procCEData;

    vmiRegInfoCP regDest = NULL;
    octiaRegListP regList = NULL;

    // get instruction attributes
    octiaAttrP attrs =
        vmiiaGetAttrs(processor, thisPC, SELECT_ATTRS, False);

    if (firstInBlock) {
        // New morph block - no information about previous morph time
        // instruction
        CEData->prevClassMT = IC_initial;
        CEData->regDestMT = NULL;
    }

    armCondition condCode;
    instrClassesE iClass =
        getInstructionClass(object, thisPC, attrs, &condCode);

    if ((regList = ocliaGetFirstReadReg(attrs))!=NULL) {
        for (; regList; regList = ocliaGetRegListNext(regList)){
            vmiRegInfoCP reg = vmiiaConvertRegInfo(ocliaGetRegListReg(regList));
            vmimtArgSimAddress(thisPC);
            vmimtArgNatAddress(object);
            vmimtArgNatAddress(reg);
            vmimtCall((vmiCallFn)addRegCount);
        }
    }

    if ((regList = ocliaGetFirstWrittenReg(attrs))!=NULL) {
        for (; regList; regList = ocliaGetRegListNext(regList)){
            vmiRegInfoCP reg = vmiiaConvertRegInfo(ocliaGetRegListReg(regList));
            vmimtArgSimAddress(thisPC);
            vmimtArgNatAddress(object);
            vmimtArgNatAddress(reg);
            vmimtCall((vmiCallFn)addRegCount);
        }
    }

    if (attrs){
        vmimtArgSimAddress(thisPC);
        vmimtArgNatAddress(object);
        vmimtCall((vmiCallFn)addToICount);
    }
    vmimtArgNatAddress(object);
    vmimtArgUns64(iClass);
    vmimtCall((vmiCallFn)addInstCount);

    // Update previous morph time instruction info
    CEData->prevClassMT = iClass;
    CEData->regDestMT = regDest;

    // free attributes
    ocliaFreeAttrs(attrs);

    *opaque = False;
    return NULL;
}

//
// Processor specific destructor called when terminating simulation
//
static PROC_DESTRUCTOR_FN(armDestructor) {
    armCEDataP CEData = object->procCEData;

    if (CEData != NULL) {
        // report the results of cycle estimation
        if (object->enabled) {
            armReport(object);
        }

        // free memory allocated by treeSearch
        treeDestroy(instructionTableSearchTreeRoot, 0, 0);

        free(CEData);
        object->procCEData = NULL;
    }
}

static vmiRegInfoCP
getReg(vmiosObjectP object, const char *regName, Bool optional) {
    vmiRegInfoCP reg = vmiosGetRegDesc(object->processor, regName);

    if (reg == NULL && !optional) {
        vmiMessage("F", PLUGIN_PREFIX,
                "%s: Processor model is incompatible: register '%s' "
                "not found on processor.",
                object->name, regName);
        // Not Reached
    }

    return reg;
}

//
// ARMM Processor-specific enable function
//
PROC_ENABLE_FN(armCEEnable) {
    if (object->procCEData == NULL) {
        armCEDataP CEData = STYPE_CALLOC(armCEDataT);
        object->procCEData = CEData;

        // Set function callbacks for this processor
        object->procMorphCB = armCEMorphCB;
        object->procDestructor = armDestructor;

        // initialize previous instruction state
        CEData->prevClassRT = IC_initial;
        CEData->prevClassMT = IC_initial;

        // Get cpsr register (required)
        CEData->cpsr = getReg(object, "cpsr", False);

        // Add branch notifier
        //vmirtRegisterBranchNotifier(object->processor,
        // armBranchNotifier, object);
    }

    if (instructionTableSearchTreeRoot == NULL) {
        // Initialize instruction class treeSearch table for fast
        // lookups
        instructionTableP entry = &instructionClasses[0];

        while (entry->mnemonic != NULL) {
            instructionTableAdd(&instructionTableSearchTreeRoot,
                    entry++);
        }
    }
}
