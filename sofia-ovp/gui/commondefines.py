#   Fault Injection Module Graphical Interface
#
#   Options and arguments
#
#
#   Author: Felipe Rocha da Rosa

from enum import Enum
import inspect
import sys

#@@@@@
#Solve bound methods problem
import copyreg
import types

def _pickle_method(m):
    if m.__self__ is None:
        return getattr, (m.__self__.__class__, m.__func__.__name__)
    else:
        return getattr, (m.__self__, m.__func__.__name__)

copyreg.pickle(types.MethodType, _pickle_method)
#@@@@@

# FIM Message interface
class fimMessages():
    def __init__(self):
        self.fim="(FIM)"

    def errorMessageWithTermination(self, message=""):
        (frame, filename, linenumber,functionname, lines, index) = inspect.getouterframes(inspect.currentframe())[1]
        print(("{0} Unrecoverable Error at {1}:{2} ({3}) {4}".format(self.fim,filename,linenumber,functionname,message)))
        sys.exit(-1)

    def debugMessage(self, message="", level=1):
        (frame, filename, linenumber,functionname, lines, index) = inspect.getouterframes(inspect.currentframe())[1]
        print(("{0} {1}".format(self.fim,message)))
        
    #Messages
    msgintflow="""\
    Introduction
    The flow bla bla bla
    
    Default Workload    Rodinia
    Default Application backprop
    Default Environment OVP ARM Cortex A9-MPx1
    \n"""
  
    symbolusage="""\
    Symbol translation table:
    COLUMN ONE: the symbol's value
    COLUMN TWO: a set of characters and spaces indicating the flag bits that are set on the symbol. There are seven groupings which are listed below:
    group one: (l,g,,!) local, global, neither, both.
    group two: (w,) weak or strong symbol.
    group three: (C,) symbol denotes a constructor or an ordinary symbol.
    group four: (W,) symbol is warning or normal symbol.
    group five: (I,) indirect reference to another symbol or normal symbol.
    group six: (d,D,) debugging symbol, dynamic symbol or normal symbol.
    group seven: (F,f,O,) symbol is the name of function, file, object or normal symbol.
    COLUMN THREE: the section in which the symbol lives, ABS means not associated with a certain section
    COLUMN FOUR: the symbol's size or alignment.
    COLUMN FIVE: the symbol's name.
    \n"""
    readelfusage="""
    Write later
    \n"""


# Pseudo Enumerator classes
class workloadsE(Enum):
    baremetal,      \
    freertos,       \
    linux,          \
    rodinia,        \
    npbser,         \
    npbomp,         \
    npbmpi = list(range(7))

class faultTypesE(Enum):
    register,       \
    memory,         \
    memory2,        \
    robsource,      \
    robdest,        \
    variable,       \
    variable2,      \
    functiontrace,  \
    functiontrace2, \
    functioncode = list(range(10))

class compilationOptsE(Enum):
    useclean,       \
    dontcompile,    \
    justcompile,    \
    append = list(range(4))

class environmentsE(Enum):
    riscv,          \
    ovparmv7,       \
    ovparmv8,       \
    ovpmips,        \
    gem5armv7,      \
    gem5armv8 = list(range(6))

class architecturesE(Enum):
    riscv,          \
    cortexA9,       \
    cortexA53,      \
    cortexA57,      \
    cortexA72,      \
    cortexM4 = list(range(6))

environmentsL    = [item.name for item in environmentsE]
workloadsL       = [item.name for item in workloadsE]
faulttypesL      = [item.name for item in faultTypesE]
architecturesL   = [item.name for item in architecturesE]
compilationoptsL = [item.name for item in compilationOptsE]

# other common lists
npbworkloadsL=[workloadsE.npbser.name, workloadsE.npbmpi.name, workloadsE.npbomp.name]
linuxworkloadsL=[workloadsE.npbser.name, workloadsE.npbmpi.name, workloadsE.npbomp.name, workloadsE.rodinia.name, workloadsE.linux.name, ]
# Fault generation 
registerfaultsL = [ faultTypesE.register.name,  faultTypesE.functiontrace.name, faultTypesE.functiontrace2.name ]
memoryfaultsL = [ faultTypesE.memory.name,  faultTypesE.memory2.name, faultTypesE.variable2.name, faultTypesE.functioncode.name ]

# Fault Injection
symbolfaultsL = [ faultTypesE.functiontrace2.name, faultTypesE.variable2.name, faultTypesE.functioncode.name ] 


