#!/usr/bin/python

# my imports
import sys
import os

import subprocess
import linecache
import ctypes
import string
from array import *
from enum import Enum
from optparse   import OptionParser
from random     import randint
from array      import *
import copy
import re
from operator import itemgetter
from collections import Counter

#python common class
from classification import *

# -------------------------------------- Fault Campaign options ------------------------------------- #
usage = "usage: %prog [options] arg1 arg2" # @Review
parser = OptionParser(usage=usage)

#Options and arguments

parser.add_option("--executiontime",      action="store", type="float",  dest="executiontime",     help="Execution time of the entire fault injection campaign (nanoseconds)", default=1000000000)
parser.add_option("--numberoffaults",     action="store", type="int",    dest="numberoffaults",    help="Number of faults to be injected in one platform",                     default=1)
parser.add_option("--parallelexecutions", action="store", type="int",    dest="parallelexecutions",help="Number of parallel platform executions",                              default=1)
parser.add_option("--cores",              action="store", type="int",    dest="cores",             help="Number of target cores",                                              default=1)
parser.add_option("--application",        action="store", type="string", dest="application",       help="Application name",                                                    default="application")
parser.add_option("--cputype",            action="store", type="string", dest="cputype",           help="Cpu type",                                                            default="ovp")
parser.add_option("--faultlist",          action="store", type="string", dest="faultlist",         help="File containing the fault list [default: %default]",                  default="./faultlist")
parser.add_option("--goldinformation",    action="store", type="string", dest="goldinformation",   help="File containing the golden information [default: %default]",          default="./goldinformation")
parser.add_option("--folder",             action="store", type="string", dest="folder",            help="Folder that contains the reports to be harvested",                    default="./Reports/")

# architecture
parser.add_option("--environment",         type="choice", dest="environment", choices=architectures, help="Choose one architecture mode", default=architectures[0])

# simulator environment

# parse the arguments
(options, args) = parser.parse_args()

# list of possible registers
archRegisters = archRegisters()

# registers for each simulator environment
possibleRegisters = archRegisters.getListOfpossibleRegisters(options,parser)
numberOfRegisters = archRegisters.getNumberOfRegisters()

####################################################################################################
# Fault Harvest                                                                                    #
####################################################################################################
faultList=[]

# expected number of faults
numberTotalOfFaults = options.numberoffaults * options.parallelexecutions

instructionCount = subprocess.check_output('grep "Application # of instructions" {0} | cut -d \'=\' -f2'.format(options.goldinformation),shell=True,).rstrip()

numberApplicationsPlotted = 0

# collect all faults
for filename in os.listdir(options.folder):
    #~ print filename
    numberApplicationsPlotted+=1
    # Collect the faults
    for i in range(2,options.numberoffaults+2):
        if not linecache.getline(options.folder+filename,i) == '': # skip empty lines
            faultList.append(linecache.getline(options.folder+filename,i).split())
        else:
            numberApplicationsPlotted-=1

# sort the list by fault index
faultList.sort(key=lambda x: int(x[0]))

# see if all fault reports are accessible
if(numberApplicationsPlotted != options.parallelexecutions):
    print "error - missing files " + str(numberTotalOfFaults-numberApplicationsPlotted)

# the files missing are considered as hangs in the ovp and unexpected termination in the gem5
if (numberApplicationsPlotted != options.parallelexecutions):
    a=0

    # collect the faults from the fault list
    for i in range(1,numberTotalOfFaults+1):
        if int(faultList[a][0]) == i:
            a+=(1 if a<(len(faultList)-1) else 0)
        else: # include missing faults
            if options.environment==archtecturesE.gem5armv7.name or options.environment==archtecturesE.gem5armv8.name:
                temp =str(linecache.getline(options.faultlist,i+1).rstrip()+" "+errorAnalysis.RD_PRIV.name+" "+str(errorAnalysis.RD_PRIV.value)+" 0 "+instructionCount+" Null 0")
                faultList.append(temp.split())
                numberApplicationsPlotted+=1
                faultList[-1][4] = faultList[-1][4].split(":")[0]
            else:
                temp =str(linecache.getline(options.faultlist,i+1).rstrip()+" "+errorAnalysis.Hang.name+" "+str(errorAnalysis.Hang.value)+" 0 "+instructionCount+" Null 0")
                faultList.append(temp.split())
                numberApplicationsPlotted+=1
                faultList[-1][4] = faultList[-1][4].split(":")[0]

# -------------------------------------- Making some calculation ------------------------------------ #
# total number of REALLY executed instructions
totalExecutedInstructions=int(sum([int(item[9]) for item in faultList]))
mipsExecutedInstructions =float(totalExecutedInstructions)/(options.executiontime /1000)

totalExecutedInstructionsWithChkp=0
for fault in faultList:
    if(fault[9]=="0"):
        print fault
    totalExecutedInstructionsWithChkp += int(fault[11]) + int(fault[9])

mipsExecutedInstructionsWithChkp =float(totalExecutedInstructionsWithChkp)/(options.executiontime /1000)

# -------------------------------------- Generate the Output File ----------------------------------- #
fileptr = open(options.application+"."+options.environment+".reportfile", 'w')

# header file
fileptr.write("%8s %7s %10s %2s %18s %17s %28s %6s %25s %22s %18s %12s\n"%(
        "Index","Type","Target","i","Insertion Time","Mask","Fault Injection Result",
        "Code", "Execution Time (Ticks)", "Executed Instructions", "Mem Inconsistency",
        "Checkpoint"))

# print faults
for faultNumber in range(0,len(faultList)):
    faultIndex                  = faultList[faultNumber][0]
    faultType                   = faultList[faultNumber][1]
    faultRegister               = faultList[faultNumber][2]
    faultRegisterIndex          = faultList[faultNumber][3]
    faultTime                   = faultList[faultNumber][4]
    faultMask                   = faultList[faultNumber][5]
    faultAnalysis               = faultList[faultNumber][6]
    faultAnalysisCode           = faultList[faultNumber][7]
    faultTicks                  = faultList[faultNumber][8]
    faultExecutedInstructions   = faultList[faultNumber][9]
    faultMemInconsistency       = faultList[faultNumber][10]
    faultCorrectCheckpoint      = faultList[faultNumber][11]

    fileptr.write("%7s %10s %6s %4s %15s %24s %25s %7s %20s %22s %14s %18s\n" % (
                    faultIndex,
                    faultType,
                    faultRegister,
                    faultRegisterIndex,
                    faultTime,
                    faultMask,
                    faultAnalysis,
                    faultAnalysisCode,
                    faultTicks,
                    faultExecutedInstructions,
                    faultMemInconsistency,
                    faultCorrectCheckpoint
                    ))

spacing = 40

# additional information @Review
fileptr.write("\n\n")
fileptr.write("%s %s\n" %("Application".ljust(spacing),options.application))
fileptr.write("%s %d\n" %("Recovered faults".ljust(spacing),numberTotalOfFaults))
fileptr.write("%s %d\n" %("Target Cores".ljust(spacing),options.cores))
fileptr.write("%s %s\n" %("Environment".ljust(spacing),options.environment))
fileptr.write("%s %s\n" %("CPU type".ljust(spacing),options.cputype))

fileptr.write("%s %d\n" %("Executed Instructions".ljust(spacing),totalExecutedInstructions))
fileptr.write("%s %d\n" %("Total Emulated".ljust(spacing),totalExecutedInstructionsWithChkp))
fileptr.write("%s %d\n" %("Gold Executed Instructions".ljust(spacing),int(instructionCount)))

fileptr.write("%s %.3f\n" %("Simulation Time (seconds)".ljust(spacing),options.executiontime/1000000000.0))
fileptr.write("%s %.3f\n" %("Million instructions per second (MIPS)".ljust(spacing),mipsExecutedInstructions))
fileptr.write("%s %.3f\n" %("MIPS if with checkpoint".ljust(spacing),mipsExecutedInstructionsWithChkp))

fileptr.close()

