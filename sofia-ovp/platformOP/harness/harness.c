
////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                           Version eng.20160705.0
//
////////////////////////////////////////////////////////////////////////////////
#include "faultInjector.c"

#include <string.h>
#include <stdlib.h>

#include "op/op.h"


#define MODULE_NAME "FIM"

typedef struct optModuleObjectS {
    // insert module persistent data here
   
} optModuleObject;


////////////////////////////////////////////////////////////////////////////////
//                         U S E R   F U N C T I O N S
////////////////////////////////////////////////////////////////////////////////

// forward declaration of component constructor
static OP_CONSTRUCT_FN(instantiateComponents);

static OP_CONSTRUCT_FN(moduleConstructor) {

    // instantiate module components
    instantiateComponents(mi, object);

    // insert constructor code here
    //~ initialize();
}

//~ static OP_PRE_SIMULATE_FN(modulePreSimulate) {
// insert pre simulation code here
//~ }

//~ static OP_SIMULATE_STARTING_FN(moduleSimulateStart) {
//~ // insert simulation starting code here
//~ }

static OP_POST_SIMULATE_FN(modulePostSimulate) {
// insert post simulation  code here
}

static OP_DESTRUCT_FN(moduleDestruct) {
// insert destructor code here
}

#include "harness.igen.h"

////////////////////////////////////////////////////////////////////////////////
//                                   M A I N
////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char *argv[]) {

    opSessionInit(OP_VERSION);
    optCmdParserP parser = opCmdParserNew(MODULE_NAME, OP_AC_ALL);

    cmdParserAddUserArgs(parser);

    opCmdParseArgs(parser, argc, argv);

    optModuleP mi = opRootModuleNew(&modelAttrs, MODULE_NAME, 0);

    ///FIM
    fimSimulate(options,mi);
    opSessionTerminate();

    return 0;
}

