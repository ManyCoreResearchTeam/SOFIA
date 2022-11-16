#My imports
import sys

import subprocess
import linecache
import ctypes

from optparse   import OptionParser
from random     import randint
from array      import *

from array import *
from enum import Enum

#List of possible registers
possibleRegisters=[]

class archRegisters:
    def __init__(self):
        self.possibleRegisters=0
        self.listOfpossibleRegisters=[]

    def getListOfpossibleRegisters(self,options,parser):
        if options.environment==archtecturesE.ovparmv7.name:
            self.listOfpossibleRegisters = ["r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12","sp","lr","pc"]
        elif options.environment==archtecturesE.gem5armv7.name:
            self.listOfpossibleRegisters = ["r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12","r13","r14","pc"]
        elif options.environment==archtecturesE.ovparmv8.name:
            self.listOfpossibleRegisters = ["x0","x1","x2","x3","x4","x5","x6","x7","x8","x9","x10","x11","x12","x13","x14","x15","x16","x17","x18","x19","x20","x21","x22","x23","x24","x25","x26","x27","x28","x29","x30","sp","pc"]
        elif options.environment==archtecturesE.gem5armv8.name:
            self.listOfpossibleRegisters = ["x0","x1","x2","x3","x4","x5","x6","x7","x8","x9","x10","x11","x12","x13","x14","x15","x16","x17","x18","x19","x20","x21","x22","x23","x24","x25","x26","x27","x28","x29","x30","sp","pc"]
        elif options.environment==archtecturesE.ovpmips.name:
            self.listOfpossibleRegisters = ["at","v0","v1","a0","a1","a2","a3","t0","t1","t2","t3","t4","t5","t6","t7","s0","s1","s2","s3","s4","s5","s6","s7","t8","t9","k0","k1","gp","sp","s8","ra","pc"]
        elif options.environment==archtecturesE.riscv.name:
            self.listOfpossibleRegisters = ["ra","sp","gp","tp","t0","t1","t2","s0","s1","a0","a1","a2","a3","a4","a5","a6","a7","s2","s3","s4","s5","s6","s7","s8","s9","s10","s11","t3","t4","t5","t6","pc"]
        else:
            parser.error("One environment is required")

        self.numberOfRegisters = len(self.listOfpossibleRegisters)-1

        return self.listOfpossibleRegisters

    def getNumberOfRegisters(self):
        return (self.numberOfRegisters+1)

class errorAnalysis(Enum):
    Masked_Fault,\
    Control_Flow_Data_OK,\
    Control_Flow_Data_ERROR,\
    REG_STATE_Data_OK,\
    REG_STATE_Data_ERROR,\
    silent_data_corruption,\
    silent_data_corruption2,\
    silent_data_corruption3,\
    RD_PRIV,\
    WR_PRIV,\
    RD_ALIGN,\
    WR_ALIGN,\
    FE_PRIV,\
    FE_ABORT,\
    SEG_FAULT,\
    ARITH,\
    Hard_Fault,\
    Lockup,\
    Unknown,\
    Hang = list(range(20))

class errorAnalysisDAC(Enum):
    Vanished,\
    Application_Output_Not_Affected,\
    Application_Output_Mismatch,\
    Unexpected_Termination,\
    Hang = list(range(5))

class errorAnalysisDACShort(Enum):
    Vanish,\
    ONA,\
    OMM,\
    UT,\
    Hang = list(range(5))

class simulationStatesE (Enum):
    FIM_GOLD_EXECUTION,\
    FIM_GOLD_PROFILING,\
    FIM_BEFORE_INJECTION,\
    FIM_AFTER_INJECTION,\
    FIM_WAIT_HANG,\
    FIM_CREATE_FAULT,\
    FIM_NO_INJECTION = list(range(7))

class faultTypesE(Enum):
    register,\
    memory,\
    robsource,\
    robdest,\
    variable,\
    functiontrace = list(range(6))

class archtecturesE(Enum):
    riscv,\
    ovparmv7,\
    ovparmv8,\
    ovpmips,\
    gem5armv7,\
    gem5armv8 = list(range(6))

faultTypes    = [item.name for item in faultTypesE]
architectures = [item.name for item in archtecturesE]
