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
#include "iattr.h"

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

typedef struct iattrInfoS {
    Uns64           thisPC;
    char           *rawDisass;
    char           *instrName;
    octiaAttrP      ia;
    iattrRegListT   srcRegs;
    iattrRegListT   dstRegs;
} iattrInfo;

typedef struct iattrCacheRootS {
    optProcessorP processor; // Processor
    optModeP      mode;      // Processor mode
    void         *treeRoot;  // Root of tree with cached octiaAttr data
} iattrCacheRootT, *iattrCacheRootP, **iattrCacheRootPP;

typedef struct iattrCacheEntryS {
    Uns32      opCodeSize;  // Size of opcode (only 16 and 32 bits supported)
    Uns32      opCode;      // Opcode value
    octiaAttrP attrs;       // pointer to octiaAttr structure for this opcode/mode
} iattrCacheEntryT, *iattrCacheEntryP, **iattrCacheEntryPP;


Bool iattrIsBranch(iattrInfoP attrs) {
	octiaInstructionClass instrClassesE = ocliaGetInstructionClass(attrs->ia);

	return ((instrClassesE & IC_branch) != 0);
}	

	
static instructionTableT instructionClasses[] = {

    // Conditional Branch
    
    	{IC_branch, "al"},
	{IC_branch, "b"},
	{IC_branch, "beq"},
	{IC_branch, "bge"},
	{IC_branch, "bgeu"},
	{IC_branch, "bgt"},
	{IC_branch, "bkpt"},
	{IC_branch, "bl"},
	{IC_branch, "ble"},
	{IC_branch, "blr"},
	{IC_branch, "blt"},
	{IC_branch, "bltu"},
	{IC_branch, "blx"},
	{IC_branch, "bne"},
	{IC_branch, "br"},
	{IC_branch, "bx"},
	{IC_branch, "cbnz"},
	{IC_branch, "cbz"},
	{IC_branch, "cc"},
	{IC_branch, "clrex"},
	{IC_branch, "cps"},
	{IC_branch, "cs"},
	{IC_branch, "dsb"},
	{IC_branch, "eq"},
	{IC_branch, "ge"},
	{IC_branch, "gt"},
	{IC_branch, "hi"},
	{IC_branch, "hs"},
	{IC_branch, "it"},
	{IC_branch, "ite"},
	{IC_branch, "itee"},
	{IC_branch, "iteee"},
	{IC_branch, "itet"},
	{IC_branch, "itete"},
	{IC_branch, "itett"},
	{IC_branch, "itt"},
	{IC_branch, "ittee"},
	{IC_branch, "ittt"},
	{IC_branch, "ittte"},
	{IC_branch, "itttt"},
	{IC_branch, "le"},
	{IC_branch, "lo"},
	{IC_branch, "ls"},
	{IC_branch, "lt"},
	{IC_branch, "mi"},
	{IC_branch, "msrcpsr"},
	{IC_branch, "msrspsr"},
	{IC_branch, "ne"},
	{IC_branch, "nv"},
	{IC_branch, "pl"},
	{IC_branch, "ret"},
	{IC_branch, "setend"},
	{IC_branch, "sev"},
	{IC_branch, "smc"},
	{IC_branch, "svc"},
	{IC_branch, "tbnz"},
	{IC_branch, "tbz"},
	{IC_branch, "vc"},
	{IC_branch, "vs"},
	{IC_branch, "wfe"},
	{IC_branch, "wfi"},

  
    // Memory load
    
	{IC_memoryLoad, "lbu"},
	{IC_memoryLoad, "ld"},
	{IC_memoryLoad, "ldar"},
	{IC_memoryLoad, "ldarb"},
	{IC_memoryLoad, "ldarh"},
	{IC_memoryLoad, "ldaxp"},
	{IC_memoryLoad, "ldaxr"},
	{IC_memoryLoad, "ldaxrb"},
	{IC_memoryLoad, "ldaxrh"},
	{IC_arithLogic, "ldc2l"}, 
	{IC_arithLogic, "ldcl"}, 
	{IC_memoryLoad, "ldm"},
	{IC_memoryLoad, "ldmdb"},
	{IC_memoryLoad, "ldmib"},
	{IC_memoryLoad, "ldnp"},
	{IC_memoryLoad, "ldp"},
	{IC_memoryLoad, "ldpsw"},
	{IC_memoryLoad, "ldr"},
	{IC_memoryLoad, "ldrb"},
	{IC_memoryLoad, "ldrd"},
	{IC_memoryLoad, "ldrex"},
	{IC_memoryLoad, "ldrh"},
	{IC_memoryLoad, "ldrsb"},
	{IC_memoryLoad, "ldrsbt"}, 
	{IC_memoryLoad, "ldrsh"},
	{IC_memoryLoad, "ldrsw"},
	{IC_memoryLoad, "ldrt"},
	{IC_memoryLoad, "ldtr"},
	{IC_memoryLoad, "ldtrb"},
	{IC_memoryLoad, "ldtrh"},
	{IC_memoryLoad, "ldtrsb"},
	{IC_memoryLoad, "ldtrsh"},
	{IC_memoryLoad, "ldtrsw"},
	{IC_memoryLoad, "ldur"},
	{IC_memoryLoad, "ldurb"},
	{IC_memoryLoad, "ldurh"},
	{IC_memoryLoad, "ldursb"},
	{IC_memoryLoad, "ldursh"},
	{IC_memoryLoad, "ldursw"},
	{IC_memoryLoad, "ldxp"},
	{IC_memoryLoad, "ldxr"},
	{IC_memoryLoad, "ldxrb"},
	{IC_memoryLoad, "ldxrh"},
	{IC_memoryLoad, "lui"},
	{IC_memoryLoad, "lw"},
	{IC_memoryLoad, "lwu"},
	{IC_memoryLoad, "pop"},
	{IC_memoryStore, "push"},
	{IC_memoryStore, "srs"},
	{IC_memoryStore, "srsdb"}, 
	{IC_memoryStore, "stlr"},
	{IC_memoryStore, "stlrb"},
	{IC_memoryStore, "stlrh"},
	{IC_memoryStore, "stlxp"},
	{IC_memoryStore, "stlxr"},
	{IC_memoryStore, "stlxrb"},
	{IC_memoryStore, "stlxrh"},
	{IC_memoryStore, "stm"},
	{IC_memoryStore, "stmdb"},
	{IC_memoryStore, "stmia"},
	{IC_memoryStore, "stmib"},
	{IC_memoryStore, "stnp"},
	{IC_memoryStore, "stp"},
	{IC_memoryStore, "str"},
	{IC_memoryStore, "strb"},
	{IC_memoryStore, "strbnet"},
	{IC_memoryStore, "strd"},
	{IC_memoryStore, "strex"},
	{IC_memoryStore, "strexeq"},
	{IC_memoryStore, "strh"},
	{IC_memoryStore, "sttr"},
	{IC_memoryStore, "sttrb"},
	{IC_memoryStore, "sttrh"},
	{IC_memoryStore, "stur"},
	{IC_memoryStore, "sturb"},
	{IC_memoryStore, "sturh"},
	{IC_memoryStore, "stxp"},
	{IC_memoryStore, "stxr"},
	{IC_memoryStore, "stxrb"},
	{IC_memoryStore, "stxrh"},
	{IC_memoryStore, "sw"},
	{IC_memory, "pld"},
	{IC_memory, "pldw"},
	{IC_memory, "prfm"},
	{IC_memory, "prfum"},


    // Data Processing

	{IC_arithLogic, "adc"},
	{IC_arithLogic, "adcs"},
	{IC_arithLogic, "add"},
	{IC_arithLogic, "addi"},
	{IC_arithLogic, "addiw"},
	{IC_arithLogic, "adds"},
	{IC_arithLogic, "addw"},
	{IC_arithLogic, "adr"},
	{IC_arithLogic, "adrp"},
	{IC_arithLogic, "and"},
	{IC_arithLogic, "andeq"},
	{IC_arithLogic, "andi"},
	{IC_arithLogic, "ands"},
	{IC_arithLogic, "asr"},
	{IC_arithLogic, "asrs"},
	{IC_arithLogic, "asrv"},
	{IC_arithLogic, "bfc"},
	{IC_arithLogic, "bfi"},
	{IC_arithLogic, "bfm"},
	{IC_arithLogic, "bfxil"},
	{IC_arithLogic, "bic"},
	{IC_arithLogic, "bics"},
	{IC_arithLogic, "ccmn"},
	{IC_arithLogic, "ccmp"},
	{IC_arithLogic, "cinc"},
	{IC_arithLogic, "cinv"},
	{IC_arithLogic, "cls"},
	{IC_arithLogic, "clz"},
	{IC_arithLogic, "cmn"},
	{IC_arithLogic, "cmp"},
	{IC_arithLogic, "cneg"},
	{IC_arithLogic, "csel"},
	{IC_arithLogic, "cset"},
	{IC_arithLogic, "csetm"},
	{IC_arithLogic, "csinc"},
	{IC_arithLogic, "csinv"},
	{IC_arithLogic, "csneg"},
	{IC_arithLogic, "eon"},
	{IC_arithLogic, "eor"},
	{IC_arithLogic, "eors"},
	{IC_arithLogic, "extr"},
	{IC_arithLogic, "lsl"},
	{IC_arithLogic, "lsls"},
	{IC_arithLogic, "lslv"},
	{IC_arithLogic, "lsr"},
	{IC_arithLogic, "lsrs"},
	{IC_arithLogic, "lsrv"},
	{IC_arithLogicMov, "mov"},
	{IC_arithLogicMov, "movi"},
	{IC_arithLogicMov, "movk"},
	{IC_arithLogicMov, "movn"},
	{IC_arithLogicMov, "movs"},
	{IC_arithLogicMov, "movt"},
	{IC_arithLogicMov, "movw"},
	{IC_arithLogicMov, "movz"},
	{IC_arithLogic, "mrs"},
	{IC_arithLogic, "msr"},
	{IC_arithLogic, "mvn"},
	{IC_arithLogicMov, "mvns"},
	{IC_arithLogicMov, "mvnsne"},
	{IC_arithLogic, "neg"},
	{IC_arithLogic, "negs"},
	{IC_arithLogic, "ngc"},
	{IC_arithLogic, "ngcs"},
	{IC_arithLogic, "ori"},
	{IC_arithLogic, "orn"},
	{IC_arithLogic, "orr"},
	{IC_arithLogic, "orrs"},
	{IC_arithLogic, "pkbht"},
	{IC_arithLogic, "pkhbt"},
	{IC_arithLogic, "pkhtb"},
	{IC_arithLogic, "qadd8"},
	{IC_arithLogic, "qadd16"},
	{IC_arithLogic, "qadd"},
	{IC_arithLogic, "qasx"},
	{IC_arithLogic, "qdadd"},
	{IC_arithLogic, "qdsub"},
	{IC_arithLogic, "qsax"},
	{IC_arithLogic, "qsub8"},
	{IC_arithLogic, "qsub"},
	{IC_arithLogic, "rbit"},
	{IC_arithLogic, "rev16"},
	{IC_arithLogic, "rev32"},
	{IC_arithLogic, "rev"},
	{IC_arithLogic, "revsh"},
	{IC_arithLogic, "ror"},
	{IC_arithLogic, "rorv"},
	{IC_arithLogic, "rrx"},
	{IC_arithLogic, "rsb"},
	{IC_arithLogic, "rsbs"},
	{IC_arithLogic, "rsc"},
	{IC_arithLogic, "sadd8"},
	{IC_arithLogic, "sadd16"},
	{IC_arithLogic, "sasx"},
	{IC_arithLogic, "sbc"},
	{IC_arithLogic, "sbcs"},
	{IC_arithLogic, "sbfiz"},
	{IC_arithLogic, "sbfm"},
	{IC_arithLogic, "sbfx"},
	{IC_arithLogicMov, "sel"},
	{IC_arithLogic, "shadd8"},
	{IC_arithLogic, "shadd16"},
	{IC_arithLogic, "shasx"},
	{IC_arithLogic, "shsax"},
	{IC_arithLogic, "shsub8"},
	{IC_arithLogic, "shsub16"},
	{IC_arithLogic, "sll"},
	{IC_arithLogic, "slli"},
	{IC_arithLogic, "slliw"},
	{IC_arithLogic, "sra"},
	{IC_arithLogic, "srai"},
	{IC_arithLogic, "sraiw"},
	{IC_arithLogic, "srl"},
	{IC_arithLogic, "srli"},
	{IC_arithLogic, "srliw"},
	{IC_arithLogic, "srlw"},
	{IC_arithLogic, "ssat16"},
	{IC_arithLogic, "ssat"},
	{IC_arithLogic, "ssax"},
	{IC_arithLogic, "ssub8"},
	{IC_arithLogic, "ssub16"},
	{IC_arithLogic, "stxb16"},
	{IC_arithLogic, "sub"},
	{IC_arithLogic, "subs"},
	{IC_arithLogic, "subw"},
	{IC_arithLogic, "sxt"},
	{IC_arithLogic, "sxtab16"},
	{IC_arithLogic, "sxtab"},
	{IC_arithLogic, "sxtah"},
	{IC_arithLogic, "sxtb16"},
	{IC_arithLogic, "sxtb"},
	{IC_arithLogic, "sxth"},
	{IC_arithLogic, "sxtw"},
	{IC_arithLogic, "teq"},
	{IC_arithLogic, "tst"},
	{IC_arithLogic, "uadd8"},
	{IC_arithLogic, "uadd16"},
	{IC_arithLogic, "uasx"},
	{IC_arithLogic, "ubfiz"},
	{IC_arithLogic, "ubfm"},
	{IC_arithLogic, "ubfx"},
	{IC_arithLogic, "uhadd8"},
	{IC_arithLogic, "uhadd16"},
	{IC_arithLogic, "uhasx"},
	{IC_arithLogic, "uhsax"},
	{IC_arithLogic, "uhsub8"},
	{IC_arithLogic, "uhsub16"},
	{IC_arithLogic, "uqadd8"},
	{IC_arithLogic, "uqasx"},
	{IC_arithLogic, "uqsax"},
	{IC_arithLogic, "uqsub8"},
	{IC_arithLogic, "uqsub16"},
	{IC_arithLogic, "usat16"},
	{IC_arithLogic, "usat"},
	{IC_arithLogic, "usax"},
	{IC_arithLogic, "usub8"},
	{IC_arithLogic, "usub16"},
	{IC_arithLogic, "utxb16"},
	{IC_arithLogic, "uxt"},
	{IC_arithLogic, "uxtab16"},
	{IC_arithLogic, "uxtab"},
	{IC_arithLogic, "uxtah"},
	{IC_arithLogic, "uxtb"},
	{IC_arithLogic, "uxth"},
	{IC_arithLogic, "xor"},
	{IC_arithLogic, "xori"},	
	                       
    // Multiply / Divide
    
   	{IC_multDiv, "madd"},
	{IC_multDiv, "mla"},
	{IC_multDiv, "mneg"},
	{IC_multDiv, "msub"},
	{IC_multDiv, "mul"},
	{IC_multDiv, "muls"},
	{IC_multDiv, "sdiv"},
	{IC_multDiv, "smaddl"},
	{IC_multDiv, "smlabb"},
	{IC_multDiv, "smlad"},
	{IC_multDiv, "smladx"},
	{IC_multDiv, "smlal"},
	{IC_multDiv, "smlald"},
	{IC_multDiv, "smlaldx"},
	{IC_multDiv, "smlalxy"},
	{IC_multDiv, "smlawy"},
	{IC_multDiv, "smlaxy"},
	{IC_multDiv, "smldldx"},
	{IC_multDiv, "smlsd"},
	{IC_multDiv, "smlsdx"},
	{IC_multDiv, "smlsld"},
	{IC_multDiv, "smmla"},
	{IC_multDiv, "smmlar"},
	{IC_multDiv, "smmls"},
	{IC_multDiv, "smmlsr"},
	{IC_multDiv, "smmul"},
	{IC_multDiv, "smmulr"},
	{IC_multDiv, "smnegl"},
	{IC_multDiv, "smsubl"},
	{IC_multDiv, "smuad"},
	{IC_multDiv, "smuadx"},
	{IC_multDiv, "smulh"},
	{IC_multDiv, "smull"},
	{IC_multDiv, "smulwy"},
	{IC_multDiv, "smulxy"},
	{IC_multDiv, "smusd"},
	{IC_multDiv, "smusdx"},
	{IC_multDiv, "udiv"},
	{IC_multDiv, "umaal"},
	{IC_multDiv, "umaddl"},
	{IC_multDiv, "umlal"},
	{IC_multDiv, "umnegl"},
	{IC_multDiv, "umsubl"},
	{IC_multDiv, "umulh"},
	{IC_multDiv, "umull"},
	
    // Floating Point
    
	{IC_fp, "fabs"},
	{IC_fp, "fadd"},
	{IC_fp, "fccmp"},
	{IC_fp, "fccmpe"},
	{IC_fp, "fcmp"},
	{IC_fp, "fcmpe"},
	{IC_fp, "fcsel"},
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
	{IC_fp, "fdiv"},
	{IC_fp, "fmadd"},
	{IC_fp, "fmax"},
	{IC_fp, "fmaxnm"},
	{IC_fp, "fmin"},
	{IC_fp, "fminnm"},
	{IC_fpMov, "fmov"},
	{IC_fp, "fmsub"},
	{IC_fp, "fmul"},
	{IC_fp, "fneg"},
	{IC_fp, "fnmadd"},
	{IC_fp, "fnmsub"},
	{IC_fp, "fnmul"},
	{IC_fp, "frinta"},
	{IC_fp, "frinti"},
	{IC_fp, "frintm"},
	{IC_fp, "frintn"},
	{IC_fp, "frintp"},
	{IC_fp, "frintx"},
	{IC_fp, "frintz"},
	{IC_fp, "fsqrt"},
	{IC_fp, "fsub"},
	{IC_fp, "scvtf"},
	{IC_fp, "ucvtf"},
	{IC_fp, "vadd"},
	{IC_fp, "vadd.f32"},
	{IC_fp, "vcvt"},
	{IC_fp, "vdiv"},
	{IC_fp, "vdiv.f32"},
	{IC_fp, "vfma.f32"},
	{IC_fp, "vldmia"},
	{IC_fp, "vldmiaeq"},
	{IC_fp, "vldr"},
	{IC_fp, "vmla"},
	{IC_fp, "vmls"},
	{IC_fp, "vmls.f32"},
	{IC_fp, "vmov"},
	{IC_fp, "vmul"},
	{IC_fp, "vmul.f32"},
	{IC_fp, "vnmla"},
	{IC_fp, "vnmls"},
	{IC_fp, "vnmul"},
	{IC_fp, "vstmia"},
	{IC_fp, "vstmiaeq"},
	{IC_fp, "vstr"},

    // Advanced SIMD

     /* { IC_SMID_               , "5.7.3"     }, */
     /* { IC_SMID_               , "5.7.4"     }, */
     /* { IC_SMID_               , "5.7.5"     }, */
     /* { IC_SMID_               , "5.7.6"     }, */
     /* { IC_SMID_               , "5.7.7"     }, */
     /* { IC_SMID_               , "5.7.8"     }, */
     /* { IC_SIMD_               , "5.7.9"     }, */
     /* { IC_SIMD_               , "5.7.10"    }, */
     /* { IC_SIMD_               , "5.7.11"    }, */
     /* { IC_SIMD_               , "5.7.12"    }, */
     /* { IC_SIMD_               , "5.7.13"    }, */
     /* { IC_SIMD_               , "5.7.14"    }, */
     /* { IC_SIMD_               , "5.7.15"    }, */
     /* { IC_SIMD_               , "5.7.18"    }, */
    /* { IC_SIMD_               , "5.7.19"    }, */
    /* { IC_SIMD_               , "5.7.20"    }, */
    /* { IC_SIMD_               , "5.7.21"    }, */
    /* { IC_SIMD_               , "5.7.22"    }, */
    /* { IC_SIMD_               , "5.7.22.1"  }, */
    /* { IC_SIMD_               , "5.7.23"    }, */
    /* { IC_SIMD_               , "5.7.24"    }, */
   
	{IC_SIMD_arithLogic, "dup"},
	{IC_SIMD_arithLogic, "ins"},
	{IC_SIMD_arithLogicMov, "smov"},
	{IC_SIMD_arithLogicMov, "umov"},
	{IC_SIMD_arithLogicMov, "vmov.f16"},
	{IC_SIMD_arithLogicMov, "vmov.f32"},
	{IC_SIMD_crypto, "aesd"},
	{IC_SIMD_crypto, "aese"},
	{IC_SIMD_crypto, "aesimc"},
	{IC_SIMD_crypto, "aesmc"},
	{IC_SIMD_crypto, "sha1c"},
	{IC_SIMD_crypto, "sha1h"},
	{IC_SIMD_crypto, "sha1m"},
	{IC_SIMD_crypto, "sha1p"},
	{IC_SIMD_crypto, "sha1su0"},
	{IC_SIMD_crypto, "sha1su1"},
	{IC_SIMD_crypto, "sha256h2"},
	{IC_SIMD_crypto, "sha256h"},
	{IC_SIMD_crypto, "sha256su0"},
	{IC_SIMD_crypto, "sha256su1"},
	{IC_SIMD_vec_arithLogic, "abs"},
	{IC_SIMD_vec_arithLogic, "addhn2"},
	{IC_SIMD_vec_arithLogic, "addhn"},
	{IC_SIMD_vec_arithLogic, "addp"},
	{IC_SIMD_vec_arithLogic, "addv"},
	{IC_SIMD_vec_arithLogic, "bif"},
	{IC_SIMD_vec_arithLogic, "bit"},
	{IC_SIMD_vec_arithLogic, "bsl"},
	{IC_SIMD_vec_arithLogic, "cmeq"},
	{IC_SIMD_vec_arithLogic, "cmge"},
	{IC_SIMD_vec_arithLogic, "cmgt"},
	{IC_SIMD_vec_arithLogic, "cmhi"},
	{IC_SIMD_vec_arithLogic, "cmhs"},
	{IC_SIMD_vec_arithLogic, "cmle"},
	{IC_SIMD_vec_arithLogic, "cmlo"},
	{IC_SIMD_vec_arithLogic, "cmls"},
	{IC_SIMD_vec_arithLogic, "cmlt"},
	{IC_SIMD_vec_arithLogic, "cmtst"},
	{IC_SIMD_vec_arithLogic, "cnt"},
	{IC_SIMD_vec_arithLogic, "ext"},
	{IC_SIMD_vec_arithLogic, "fabd"},
	{IC_SIMD_vec_arithLogic, "facge"},
	{IC_SIMD_vec_arithLogic, "facgt"},
	{IC_SIMD_vec_arithLogic, "facle"},
	{IC_SIMD_vec_arithLogic, "faclt"},
	{IC_SIMD_vec_arithLogic, "faddp"},
	{IC_SIMD_vec_arithLogic, "fcmeq"},
	{IC_SIMD_vec_arithLogic, "fcmge"},
	{IC_SIMD_vec_arithLogic, "fcmgt"},
	{IC_SIMD_vec_arithLogic, "fcmle"},
	{IC_SIMD_vec_arithLogic, "fcmlt"},
	{IC_SIMD_vec_arithLogic, "fcvtl2"},
	{IC_SIMD_vec_arithLogic, "fcvtl"},
	{IC_SIMD_vec_arithLogic, "fcvtn2"},
	{IC_SIMD_vec_arithLogic, "fcvtn"},
	{IC_SIMD_vec_arithLogic, "fcvtxn2"},
	{IC_SIMD_vec_arithLogic, "fcvtxn"},
	{IC_SIMD_vec_arithLogic, "fcvtxs"},
	{IC_SIMD_vec_arithLogic, "fmaxnmp"},
	{IC_SIMD_vec_arithLogic, "fmaxnmv"},
	{IC_SIMD_vec_arithLogic, "fmaxp"},
	{IC_SIMD_vec_arithLogic, "fmaxv"},
	{IC_SIMD_vec_arithLogic, "fminnmp"},
	{IC_SIMD_vec_arithLogic, "fminnmv"},
	{IC_SIMD_vec_arithLogic, "fminp"},
	{IC_SIMD_vec_arithLogic, "fminv"},
	{IC_SIMD_vec_arithLogic, "fmulx"},
	{IC_SIMD_vec_arithLogic, "frecpe"},
	{IC_SIMD_vec_arithLogic, "frecps"},
	{IC_SIMD_vec_arithLogic, "frecpx"},
	{IC_SIMD_vec_arithLogic, "frsqrte"},
	{IC_SIMD_vec_arithLogic, "frsqrts"},
	{IC_SIMD_vec_arithLogic, "not"},
	{IC_SIMD_vec_arithLogic, "pmul"},
	{IC_SIMD_vec_arithLogic, "pmull2"},
	{IC_SIMD_vec_arithLogic, "pmull"},
	{IC_SIMD_vec_arithLogic, "raddhn2"},
	{IC_SIMD_vec_arithLogic, "raddhn"},
	{IC_SIMD_vec_arithLogic, "rev64"},
	{IC_SIMD_vec_arithLogic, "rshrn2"},
	{IC_SIMD_vec_arithLogic, "rshrn"},
	{IC_SIMD_vec_arithLogic, "rsubhn2"},
	{IC_SIMD_vec_arithLogic, "rsubhn"},
	{IC_SIMD_vec_arithLogic, "saba"},
	{IC_SIMD_vec_arithLogic, "sabal2"},
	{IC_SIMD_vec_arithLogic, "sabal"},
	{IC_SIMD_vec_arithLogic, "sabd"},
	{IC_SIMD_vec_arithLogic, "sabdl2"},
	{IC_SIMD_vec_arithLogic, "sabdl"},
	{IC_SIMD_vec_arithLogic, "sadalp"},
	{IC_SIMD_vec_arithLogic, "saddl2"},
	{IC_SIMD_vec_arithLogic, "saddl"},
	{IC_SIMD_vec_arithLogic, "saddlp"},
	{IC_SIMD_vec_arithLogic, "saddlv"},
	{IC_SIMD_vec_arithLogic, "saddw2"},
	{IC_SIMD_vec_arithLogic, "saddw"},
	{IC_SIMD_vec_arithLogic, "shadd"},
	{IC_SIMD_vec_arithLogic, "shl"},
	{IC_SIMD_vec_arithLogic, "shrn2"},
	{IC_SIMD_vec_arithLogic, "shrn"},
	{IC_SIMD_vec_arithLogic, "shsub"},
	{IC_SIMD_vec_arithLogic, "sli"},
	{IC_SIMD_vec_arithLogic, "smax"},
	{IC_SIMD_vec_arithLogic, "smaxp"},
	{IC_SIMD_vec_arithLogic, "smaxv"},
	{IC_SIMD_vec_arithLogic, "smin"},
	{IC_SIMD_vec_arithLogic, "sminp"},
	{IC_SIMD_vec_arithLogic, "sminv"},
	{IC_SIMD_vec_arithLogic, "sqabs"},
	{IC_SIMD_vec_arithLogic, "sqadd"},
	{IC_SIMD_vec_arithLogic, "sqdmlal"},
	{IC_SIMD_vec_arithLogic, "sqdmlsl2"},
	{IC_SIMD_vec_arithLogic, "sqdmlsl"},
	{IC_SIMD_vec_arithLogic, "sqdmulh"},
	{IC_SIMD_vec_arithLogic, "sqneg"},
	{IC_SIMD_vec_arithLogic, "sqrdmulh"},
	{IC_SIMD_vec_arithLogic, "sqrshl"},
	{IC_SIMD_vec_arithLogic, "sqrshrn2"},
	{IC_SIMD_vec_arithLogic, "sqrshrn"},
	{IC_SIMD_vec_arithLogic, "sqrshrun2"},
	{IC_SIMD_vec_arithLogic, "sqrshrun"},
	{IC_SIMD_vec_arithLogic, "sqshl"},
	{IC_SIMD_vec_arithLogic, "sqshlu"},
	{IC_SIMD_vec_arithLogic, "sqshrn2"},
	{IC_SIMD_vec_arithLogic, "sqshrn"},
	{IC_SIMD_vec_arithLogic, "sqshrun2"},
	{IC_SIMD_vec_arithLogic, "sqshrun"},
	{IC_SIMD_vec_arithLogic, "sqsub"},
	{IC_SIMD_vec_arithLogic, "sqxtn2"},
	{IC_SIMD_vec_arithLogic, "sqxtn"},
	{IC_SIMD_vec_arithLogic, "sqxtun2"},
	{IC_SIMD_vec_arithLogic, "sqxtun"},
	{IC_SIMD_vec_arithLogic, "srhadd"},
	{IC_SIMD_vec_arithLogic, "sri"},
	{IC_SIMD_vec_arithLogic, "srshl"},
	{IC_SIMD_vec_arithLogic, "srshr"},
	{IC_SIMD_vec_arithLogic, "srsra"},
	{IC_SIMD_vec_arithLogic, "sshl"},
	{IC_SIMD_vec_arithLogic, "sshll2"},
	{IC_SIMD_vec_arithLogic, "sshll"},
	{IC_SIMD_vec_arithLogic, "sshr"},
	{IC_SIMD_vec_arithLogic, "ssra"},
	{IC_SIMD_vec_arithLogic, "ssubl2"},
	{IC_SIMD_vec_arithLogic, "ssubl"},
	{IC_SIMD_vec_arithLogic, "ssubw2"},
	{IC_SIMD_vec_arithLogic, "ssubw"},
	{IC_SIMD_vec_arithLogic, "subhn2"},
	{IC_SIMD_vec_arithLogic, "subhn"},
	{IC_SIMD_vec_arithLogic, "suqadd"},
	{IC_SIMD_vec_arithLogic, "sxtl2"},
	{IC_SIMD_vec_arithLogic, "sxtl"},
	{IC_SIMD_vec_arithLogic, "trn1"},
	{IC_SIMD_vec_arithLogic, "trn2"},
	{IC_SIMD_vec_arithLogic, "uaba"},
	{IC_SIMD_vec_arithLogic, "uabal2"},
	{IC_SIMD_vec_arithLogic, "uabal"},
	{IC_SIMD_vec_arithLogic, "uabd"},
	{IC_SIMD_vec_arithLogic, "uabdl2"},
	{IC_SIMD_vec_arithLogic, "uabdl"},
	{IC_SIMD_vec_arithLogic, "uadalp"},
	{IC_SIMD_vec_arithLogic, "uaddl2"},
	{IC_SIMD_vec_arithLogic, "uaddl"},
	{IC_SIMD_vec_arithLogic, "uaddlp"},
	{IC_SIMD_vec_arithLogic, "uaddlv"},
	{IC_SIMD_vec_arithLogic, "uaddw2"},
	{IC_SIMD_vec_arithLogic, "uaddw"},
	{IC_SIMD_vec_arithLogic, "uhadd"},
	{IC_SIMD_vec_arithLogic, "uhsub"},
	{IC_SIMD_vec_arithLogic, "umax"},
	{IC_SIMD_vec_arithLogic, "umaxp"},
	{IC_SIMD_vec_arithLogic, "umaxv"},
	{IC_SIMD_vec_arithLogic, "umin"},
	{IC_SIMD_vec_arithLogic, "uminp"},
	{IC_SIMD_vec_arithLogic, "uminv"},
	{IC_SIMD_vec_arithLogic, "uqadd"},
	{IC_SIMD_vec_arithLogic, "uqrshl"},
	{IC_SIMD_vec_arithLogic, "uqrshrn2"},
	{IC_SIMD_vec_arithLogic, "uqrshrn"},
	{IC_SIMD_vec_arithLogic, "uqshl"},
	{IC_SIMD_vec_arithLogic, "uqshrn2"},
	{IC_SIMD_vec_arithLogic, "uqshrn"},
	{IC_SIMD_vec_arithLogic, "uqsub"},
	{IC_SIMD_vec_arithLogic, "uqxtn2"},
	{IC_SIMD_vec_arithLogic, "uqxtn"},
	{IC_SIMD_vec_arithLogic, "urecpe"},
	{IC_SIMD_vec_arithLogic, "urhadd"},
	{IC_SIMD_vec_arithLogic, "urshl"},
	{IC_SIMD_vec_arithLogic, "urshr"},
	{IC_SIMD_vec_arithLogic, "ursqrte"},
	{IC_SIMD_vec_arithLogic, "ursra"},
	{IC_SIMD_vec_arithLogic, "ushl"},
	{IC_SIMD_vec_arithLogic, "ushll2"},
	{IC_SIMD_vec_arithLogic, "ushll"},
	{IC_SIMD_vec_arithLogic, "ushr"},
	{IC_SIMD_vec_arithLogic, "usqadd"},
	{IC_SIMD_vec_arithLogic, "usra"},
	{IC_SIMD_vec_arithLogic, "usubl2"},
	{IC_SIMD_vec_arithLogic, "usubl"},
	{IC_SIMD_vec_arithLogic, "usubw2"},
	{IC_SIMD_vec_arithLogic, "usubw"},
	{IC_SIMD_vec_arithLogic, "uxtl2"},
	{IC_SIMD_vec_arithLogic, "uxtl"},
	{IC_SIMD_vec_arithLogic, "uzp1"},
	{IC_SIMD_vec_arithLogic, "uzp2"},
	{IC_SIMD_vec_arithLogic, "vabd.u8"},
	{IC_SIMD_vec_arithLogic, "vcge.f32"},
	{IC_SIMD_vec_arithLogic, "vcmpe.f64"},
	{IC_SIMD_vec_arithLogic, "vext.8"},
	{IC_SIMD_vec_arithLogic, "vhadd.s8"},
	{IC_SIMD_vec_arithLogic, "vhadd.s32"},
	{IC_SIMD_vec_arithLogic, "vhadd.u16"},
	{IC_SIMD_vec_arithLogic, "vmrs"},
	{IC_SIMD_vec_arithLogic, "vmsr"},
	{IC_SIMD_vec_arithLogic, "vpmax.s8"},
	{IC_SIMD_vec_arithLogic, "vrshl.u8"},
	{IC_SIMD_vec_arithLogic, "vshr.u32"},
	{IC_SIMD_vec_arithLogic, "vsubl.s32"},
	{IC_SIMD_vec_arithLogic, "xtn2"},
	{IC_SIMD_vec_arithLogic, "xtn"},
	{IC_SIMD_vec_arithLogic, "zip1"},
	{IC_SIMD_vec_arithLogic, "zip2"},
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
	{IC_SIMD_vec_memoryStore, "vst1.64"},
	{IC_SIMD_vec_memoryStore, "vst2.8"},
	{IC_SIMD_vec_mult, "fmla"},
	{IC_SIMD_vec_mult, "fmls"},
	{IC_SIMD_vec_mult, "mls"},
	{IC_SIMD_vec_mult, "smlal2"},
	{IC_SIMD_vec_mult, "smlsl2"},
	{IC_SIMD_vec_mult, "smlsl"},
	{IC_SIMD_vec_mult, "smull2"},
	{IC_SIMD_vec_mult, "sqdmlal2"},
	{IC_SIMD_vec_mult, "sqdmull2"},
	{IC_SIMD_vec_mult, "sqdmull"},
	{IC_SIMD_vec_mult, "umlal2"},
	{IC_SIMD_vec_mult, "umlsl2"},
	{IC_SIMD_vec_mult, "umlsl"},
	{IC_SIMD_vec_mult, "umull2"},
	{IC_SIMD_vec_tbl, "tbh"},
	{IC_SIMD_vec_tbl, "tbl"},
	{IC_SIMD_vec_tbl, "tbx"},
	{IC_SIMD_vec_tbl, "vtbl.8"},
	{IC_SIMD_vec_tbl, "vtbx.8"},

    /* { IC_system              , "5.8.1"     }, */
    /* { IC_system              , "5.8.1.1"   }, */
    /* { IC_system              , "5.8.1.2"   }, */
    /* { IC_system              , "5.8.2"     }, */
    /* { IC_system              , "5.8.3"     }, */
    /* { IC_system              , "5.8.4"     }, */
    /* { IC_system              , "5.8.5"     }, */

	{IC_system, "brk"},
	{IC_system, "cdp2"},
	{IC_system, "cdp"},
	{IC_system, "cpsid"},
	{IC_system, "cpsie"},
	{IC_system, "dbg"},
	{IC_system, "dc"},
	{IC_system, "dcps1"},
	{IC_system, "dcps2"},
	{IC_system, "dcps3"},
	{IC_system, "dmb"},
	{IC_system, "drps"},
	{IC_system, "eret"},
	{IC_system, "hlt"},
	{IC_system, "hvc"},
	{IC_system, "ic"},
	{IC_system, "isb"},
	{IC_system, "mcr2"},
	{IC_system, "mcr"},
	{IC_system, "mcrr"},
	{IC_system, "mrc2"},
	{IC_system, "mrc"},
	{IC_system, "nop"},
	{IC_system, "rfe"}, 
	{IC_system, "rfedb"}, 
	{IC_system, "sevl"},
	{IC_system, "stc2"},
	{IC_system, "stc2l"},
	{IC_system, "sys"},
	{IC_system, "sysl"},
	{IC_system, "tlbi"},
	{IC_system, "yield"},
      
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

//Structs criadas para cada conjunto de registrador

struct regLifetimeXn {
	char* name;
	int lifetime;
	int interval;
	int written;
	int read;
	int lifetimeExtra;
};

struct regLifetimeVn {
	char* name;
	int lifetime;
	int interval;
	int written;
	int read;
	int lifetimeExtra;
};

struct regLifetimeRn {
	char* name;
	int lifetime;
	int interval;
	int written;
	int read;
	int lifetimeExtra;
};

int flag = 0; //Flag que indica se um registrador está sendo lido ou escrito. 1 se lido, 2 se escrito.

//Inicialização de cada uma das structs. Elas precisam ser iniciadas, mas se forem inicadas dentro de uma função os valores são zerados

struct regLifetimeXn *regLifetime_xn() {
	
    static int initialized = 0;
    static struct regLifetimeXn *xn;
    int i = 0;
    
    if(initialized == 0){
    xn = malloc(sizeof(struct regLifetimeXn) * 31);
    	if(xn == NULL){
    	exit(1);
    	}  	
    
    for(i = 0; i<31; i++){
        xn[i].name = ""; 
    	xn[i].lifetime = 0;
    	xn[i].interval = 0;
    	xn[i].written = 0;
    	xn[i].read = 0;
    	xn[i].lifetimeExtra = 0;
    }
    
    for(i = 0; i<31; i++){
    xn[i].name = (char*) malloc((strlen(xn[i].name) + 1) * sizeof(char));
    }
    
    strcpy(xn[0].name, "x0");
    strcpy(xn[1].name, "x1");
    strcpy(xn[2].name, "x2");
    strcpy(xn[3].name, "x3");
    strcpy(xn[4].name, "x4");
    strcpy(xn[5].name, "x5");
    strcpy(xn[6].name, "x6");
    strcpy(xn[7].name, "x7");
    strcpy(xn[8].name, "x8");
    strcpy(xn[9].name, "x9");
    strcpy(xn[10].name, "x10");
    strcpy(xn[11].name, "x11");    
    strcpy(xn[12].name, "x12");
    strcpy(xn[13].name, "x13");
    strcpy(xn[14].name, "x14");
    strcpy(xn[15].name, "x15");
    strcpy(xn[16].name, "x16");
    strcpy(xn[17].name, "x17");
    strcpy(xn[18].name, "x18");
    strcpy(xn[19].name, "x19");
    strcpy(xn[20].name, "x20");
    strcpy(xn[21].name, "x21");
    strcpy(xn[22].name, "x22");
    strcpy(xn[23].name, "x23");
    strcpy(xn[24].name, "x24");
    strcpy(xn[25].name, "x25"); 
    strcpy(xn[26].name, "x26");
    strcpy(xn[27].name, "x27");
    strcpy(xn[28].name, "x28");
    strcpy(xn[29].name, "x29");
    strcpy(xn[30].name, "x30");
    
    initialized = 1;
    }
    return xn;
}

struct regLifetimeVn *regLifetime_vn() {
	
    static int initialized = 0;
    static struct regLifetimeVn *vn;
    int i = 0;
    
    if(initialized == 0){
    vn = malloc(sizeof(struct regLifetimeVn) * 32);
    	if(vn == NULL){
    	exit(1);
    	}  	
    
    for(i = 0; i<32; i++){
        vn[i].name = ""; 
    	vn[i].lifetime = 0;
    	vn[i].interval = 0;
    	vn[i].written = 0;
    	vn[i].read = 0;
    	vn[i].lifetimeExtra = 0;
    }
    
    for(i = 0; i<32; i++){
    vn[i].name = (char*) malloc((strlen(vn[i].name) + 1) * sizeof(char));
    }
    
    strcpy(vn[0].name, "v0");
    strcpy(vn[1].name, "v1");
    strcpy(vn[2].name, "v2");
    strcpy(vn[3].name, "v3");
    strcpy(vn[4].name, "v4");
    strcpy(vn[5].name, "v5");
    strcpy(vn[6].name, "v6");
    strcpy(vn[7].name, "v7");
    strcpy(vn[8].name, "v8");
    strcpy(vn[9].name, "v9");
    strcpy(vn[10].name, "v10");
    strcpy(vn[11].name, "v11");    
    strcpy(vn[12].name, "v12");
    strcpy(vn[13].name, "v13");
    strcpy(vn[14].name, "v14");
    strcpy(vn[15].name, "v15");
    strcpy(vn[16].name, "v16");
    strcpy(vn[17].name, "v17");
    strcpy(vn[18].name, "v18");
    strcpy(vn[19].name, "v19");
    strcpy(vn[20].name, "v20");
    strcpy(vn[21].name, "v21");
    strcpy(vn[22].name, "v22");
    strcpy(vn[23].name, "v23");
    strcpy(vn[24].name, "v24");
    strcpy(vn[25].name, "v25"); 
    strcpy(vn[26].name, "v26");
    strcpy(vn[27].name, "v27");
    strcpy(vn[28].name, "v28");
    strcpy(vn[29].name, "v29");
    strcpy(vn[30].name, "v30");
    strcpy(vn[31].name, "v31"); 

    initialized = 1;
    }
    return vn;
}

struct regLifetimeRn *regLifetime_rn() {
	
    static int initialized = 0;
    static struct regLifetimeRn *rn;
    int i = 0;
    
    if(initialized == 0){
    rn = malloc(sizeof(struct regLifetimeRn) * 13);
    	if(rn == NULL){
    	exit(1);
    	}
    
    	
    for(i = 0; i<13; i++){
        rn[i].name = ""; 
    	rn[i].lifetime = 0;
    	rn[i].interval = 0;
    	rn[i].written = 0;
    	rn[i].read = 0;
    	rn[i].lifetimeExtra = 0;
    }
    
    for(i = 0; i<13; i++){
    rn[i].name = (char*) malloc((strlen(rn[i].name) + 1) * sizeof(char));
    }
    
    strcpy(rn[0].name, "r0");
    strcpy(rn[1].name, "r1");
    strcpy(rn[2].name, "r2");
    strcpy(rn[3].name, "r3");
    strcpy(rn[4].name, "r4");
    strcpy(rn[5].name, "r5");
    strcpy(rn[6].name, "r6");
    strcpy(rn[7].name, "r7");
    strcpy(rn[8].name, "r8");
    strcpy(rn[9].name, "r9");
    strcpy(rn[10].name, "r10");
    strcpy(rn[11].name, "r11");    
    strcpy(rn[12].name, "r12");
    
    initialized = 1;
    }
    return rn;
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
    struct regLifetimeXn *xn = regLifetime_xn();
    struct regLifetimeVn *vn = regLifetime_vn();
    struct regLifetimeRn *rn = regLifetime_rn(); 
    

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
      	    "\n\n\nInteger Registers\n\n\nr0        " FMT_64u
            "\nr1        " FMT_64u
            "\nr2        " FMT_64u
            "\nr3        " FMT_64u
            "\nr4        " FMT_64u
            "\nr5        " FMT_64u
            "\nr6        " FMT_64u
            "\nr7        " FMT_64u
            "\nr8        " FMT_64u
            "\nr9        " FMT_64u
            "\nr10       " FMT_64u
            "\nr11       " FMT_64u
            "\nr12       " FMT_64u
            "\nlr       " FMT_64u,
        object->regR0,
        object->regR1,
        object->regR2,
        object->regR3,
        object->regR4,
        object->regR5,
        object->regR6,
        object->regR7,
        object->regR8,
        object->regR9,
        object->regR10,
        object->regR11,
        object->regR12,
        object->regLr);

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
            
           
    int i = 0;        
    
    vmiPrintf("\nLifetime\n");
    vmiPrintf("\n");
    for(i = 0; i<31; i++){
    if(xn[i].lifetime != 0)        
    vmiPrintf("x%d: %d\n", i, xn[i].lifetime);
    }
    for(i = 0; i<32; i++){      
    if(vn[i].lifetime != 0)  
    vmiPrintf("v%d: %d\n", i, vn[i].lifetime);
    }
    for(i = 0; i<13; i++){ 
    if(rn[i].lifetime != 0)       
    vmiPrintf("r%d: %d\n", i, rn[i].lifetime);
    }
    
    vmiPrintf("\nIntervals\n");
    vmiPrintf("\n");
    for(i = 0; i<31; i++){
    if(xn[i].lifetime != 0)        
    vmiPrintf("x%d: %d\n", i, xn[i].interval);
    }
    for(i = 0; i<32; i++){
    if(vn[i].lifetime != 0)        
    vmiPrintf("v%d: %d\n", i, vn[i].interval);
    }
    for(i = 0; i<13; i++){
    if(rn[i].lifetime != 0)        
    vmiPrintf("r%d: %d\n", i, rn[i].interval);
    }
     
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
        else if (strcmp(reg->name, "r0") == 0)
            object->regR0++;
        else if (strcmp(reg->name, "r1") == 0)
            object->regR1++;
        else if (strcmp(reg->name, "r2") == 0)
            object->regR2++;
        else if (strcmp(reg->name, "r3") == 0)
            object->regR3++;
        else if (strcmp(reg->name, "r4") == 0)
            object->regR4++;
        else if (strcmp(reg->name, "r5") == 0)
            object->regR5++;
        else if (strcmp(reg->name, "r6") == 0)
            object->regR6++;
        else if (strcmp(reg->name, "r7") == 0)
            object->regR7++;
        else if (strcmp(reg->name, "r8") == 0)
            object->regR8++;     
        else if (strcmp(reg->name, "r9") == 0)
            object->regR9++;     
        else if (strcmp(reg->name, "r10") == 0)
            object->regR10++;     
        else if (strcmp(reg->name, "r11") == 0)
            object->regR11++;     
        else if (strcmp(reg->name, "r12") == 0)
            object->regR12++;          
        else
            object->regOthers++;
		
    	}

}

// Emit morph code to add cycles to additionalCycles counter
//
static void
addInstCount(vmiosObjectP object, instrClassesE iClass, Addr thisPC) {

    if (object->active){

        switch (iClass) {
            case IC_default:
                printf("Instruction default: %s\n", vmirtDisassemble(object->processor, thisPC, DSA_NORMAL));
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

// A função teste é o que calcula o lifetime e o intervalo. Para o lifetime há 5 condicionais:
// 1.Registrador está sendo escrito: nesse caso uma flag será ativada
// 2.Registrador não está sendo lido ou escrito, mas se a flag estiver ativada ele ainda está vulnerável
// 3.Registrador está sendo lido. Nesse caso a flag é desativada
// 4.Registrador não está sendo lido ou escrito e a flag está desativada: uma nova variável é acrescida, pois não há como saber se o registrador será lido novamente ao longo da operação
// 5.Registrador foi lido mas a flag está desativada: a variável criada no item anterior é somada ao lifetime para considerar o tempo que o mesmo está vulnerável para além do primeiro ciclo de escrita-leitura 

long int lastPC = 0;

static void teste(Addr thisPC, vmiosObjectP object, vmiRegInfoCP reg){
	
	if(object->active){
	
		
	lastPC = thisPC;
	
	int i = 0;	

	struct regLifetimeXn *xn = regLifetime_xn(); 	
	struct regLifetimeVn *vn = regLifetime_vn(); 		
	struct regLifetimeRn *rn = regLifetime_rn(); 
	
//Xn
		for(i=0;i<31;i++){
		if((strcmp(reg->name, xn[i].name) == 0) && flag == 2){
			xn[i].interval++;
			if(xn[i].written == 0)
				xn[i].written = 1;
			if(xn[i].written == 1)
				xn[i].lifetime++;
			if(xn[i].read == 1)
				xn[i].lifetimeExtra++;
		}
		
		if((strcmp(reg->name, xn[i].name) == 0) && xn[i].written == 1 && flag == 1){
			if(lastPC != thisPC)
				xn[i].lifetime++;
			xn[i].written = 2;
			xn[i].read = 1;
		}
		
		if((strcmp(reg->name, xn[i].name) != 0) && (reg->name[0] == 'x' || reg->name[0] == 'd') && xn[i].written == 1 && flag == 2){
			xn[i].lifetime++;
		}
		
		if((strcmp(reg->name, xn[i].name) == 0) && xn[i].read == 1 && flag == 1){
			if(lastPC != thisPC)
				xn[i].lifetimeExtra++;
			xn[i].lifetime = xn[i].lifetime + xn[i].lifetimeExtra++;
			xn[i].lifetimeExtra = 0;
		}
		
		if((strcmp(reg->name, xn[i].name) != 0) && (reg->name[0] == 'x' || reg->name[0] == 'd') && xn[i].read == 1 && flag == 2){
			xn[i].lifetimeExtra++;
		}
		}
		
//Vn
		for(i=0;i<32;i++){
		if((strcmp(reg->name, vn[i].name) == 0) && flag == 2){
			vn[i].interval++;
			if(vn[i].written == 0)
				vn[i].written = 1;
			if(vn[i].written == 1)
				vn[i].lifetime++;
			if(vn[i].read == 1)
				vn[i].lifetimeExtra++;
		}
		
		if((strcmp(reg->name, vn[i].name) == 0) && vn[i].written == 1 && flag == 1){
			if(lastPC != thisPC)
				vn[i].lifetime++;
			vn[i].written = 2;
			vn[i].read = 1;
		}
		
		if((strcmp(reg->name, vn[i].name) != 0) && (reg->name[0] == 'v' || reg->name[0] == 'd') && vn[i].written == 1 && flag == 2){
			vn[i].lifetime++;
		}
		
		if((strcmp(reg->name, vn[i].name) == 0) && vn[i].read == 1 && flag == 1){
			if(lastPC != thisPC)
				vn[i].lifetimeExtra++;
			vn[i].lifetime = vn[i].lifetime + vn[i].lifetimeExtra++;
			vn[i].lifetimeExtra = 0;
		}
		
		if((strcmp(reg->name, vn[i].name) != 0) && (reg->name[0] == 'v' || reg->name[0] == 'd') && vn[i].read == 1 && flag == 2){
			vn[i].lifetimeExtra++;
		}
		}
	
//Rn 		
		
		for(i=0;i<13;i++){

		if((strcmp(reg->name, rn[i].name) == 0) && flag == 2){
			rn[i].interval++;
			if(rn[i].written == 0)
				rn[i].written = 1;
			if(rn[i].written == 1)
				rn[i].lifetime++;
			if(rn[i].read == 1)
				rn[i].lifetimeExtra++;
		}
		
		if((strcmp(reg->name, rn[i].name) == 0) && rn[i].written == 1 && flag == 1){
			if(lastPC != thisPC)
				rn[i].lifetime++;
			rn[i].written = 2;
			rn[i].read = 1;
		}
		
		if((strcmp(reg->name, rn[i].name) != 0) && (reg->name[0] == 'r' || reg->name[0] == 'd') && rn[i].written == 1 && flag == 2){
			rn[i].lifetime++;
		}
		
		if((strcmp(reg->name, rn[i].name) == 0) && rn[i].read == 1 && flag == 1){
			if(lastPC != thisPC)
				rn[i].lifetimeExtra++;
			rn[i].lifetime = rn[i].lifetime + rn[i].lifetimeExtra++;
			rn[i].lifetimeExtra = 0;
		}
		
		if((strcmp(reg->name, rn[i].name) != 0) && (reg->name[0] == 'r' || reg->name[0] == 'd') && rn[i].read == 1 && flag == 2){
			rn[i].lifetimeExtra++;
		}		
		}
		
	}	
}

#define SELECT_ATTRS                                                     \
    (OCL_DS_REG_R | OCL_DS_REG_W | OCL_DS_RANGE_R | OCL_DS_RANGE_W | \
     OCL_DS_FETCH | OCL_DS_NEXTPC | OCL_DS_ADDRESS)

VMIOS_MORPH_FN(armCEMorphCB) {

	//struct regLifetimeXn *xn = regLifetime_xn(); 	
	//struct regLifetimeVn *vn = regLifetime_vn(); 		
	//struct regLifetimeRn *rn = regLifetime_rn(); 
	
	//int i = 0;

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
            flag=1;
            teste(thisPC, object, reg);
        }

    }


    if ((regList = ocliaGetFirstWrittenReg(attrs))!=NULL) {
        for (; regList; regList = ocliaGetRegListNext(regList)){
            vmiRegInfoCP reg = vmiiaConvertRegInfo(ocliaGetRegListReg(regList));
            vmimtArgSimAddress(thisPC);
            vmimtArgNatAddress(object);
            vmimtArgNatAddress(reg);
            vmimtCall((vmiCallFn)addRegCount);
            flag=2;
            teste(thisPC, object, reg);
        }
    }

    if (attrs){
        vmimtArgSimAddress(thisPC);
        vmimtArgNatAddress(object);
        vmimtCall((vmiCallFn)addToICount); //Retorna o número de instruções executadas
    }
    vmimtArgNatAddress(object);
    vmimtArgUns64(iClass);
    vmimtCall((vmiCallFn)addInstCount); //Retorna o número dos tipos de instruções executadas


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

PROC_ENABLE_FN(armmCEEnable) {
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

PROC_ENABLE_FN(rh850CEEnable) {
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

PROC_ENABLE_FN(m5100CEEnable) {
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
