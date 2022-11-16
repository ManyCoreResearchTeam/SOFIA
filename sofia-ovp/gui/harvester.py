#   Fault Injection Module Graphical Interface
#
#
#
#
#   Author: Felipe Rocha da Rosa

# Libraries
import sys
import os

import subprocess
import linecache
import ctypes
import copy
import re
from random     import randint
from array      import *

import pylab as P
import numpy as np
from collections import namedtuple

# Local classes
from classification import *
from commondefines  import *
class harvester():
    def __init__(self,exectime=1.0,numfaults=1,numcores=1,faultlist="./faultlist",goldinformation="./goldinformation",
            environment=environmentsL[0],application=None,dirreports="./Reports",outputfolder="./"):
        self.exectime        =  exectime
        self.numfaults       =  numfaults
        self.numcores        =  numcores
        self.faultlist       =  faultlist
        self.goldinformation =  goldinformation
        self.environment     =  environment
        self.application     =  application
        self.dirreports      =  dirreports
        self.diroutput       =  outputfolder
        
        self.faultsperplatform=1
        self.cputype="OVP"

    def harvest(self):
        # list of possible registers
        archregisters = archRegisters()

        # registers for each simulator environment
        possibleRegisters = archregisters.possibleregisters(self.environment)
        numberOfRegisters = archregisters.getNumberOfRegisters()

        ####################################################################################################
        # Fault Harvest                                                                                    #
        ####################################################################################################
        faultList=[]

        instructionCount = subprocess.check_output('grep "Application # of instructions" {0} | cut -d \'=\' -f2'.format(self.goldinformation),shell=True,).rstrip()

        recoveredfaults = 0

        # collect all faults
        for filename in os.listdir(self.dirreports):
            recoveredfaults+=1
            filepath=self.dirreports+"/"+filename
            # Collect the faults
            for i in range(2,self.faultsperplatform+2):
                #~ print i,linecache.getline(filepath,i)
                if not linecache.getline(filepath,i) == '': # skip empty lines
                    faultList.append(linecache.getline(filepath,i).split())
                else:
                    recoveredfaults-=1

        # sort the list by fault index
        faultList.sort(key=lambda x: int(x[0]))
        #~ print faultList,self.dirreports

        # the files missing are considered as hangs in the ovp and unexpected termination in the gem5
        if (recoveredfaults != self.numfaults):
            print("error - missing files " + str(self.numfaults-recoveredfaults))
            a=0
        
            # collect the faults from the fault list
            for i in range(1,self.numfaults+1):
                if int(faultList[a][0]) == i:
                    a+=(1 if a<(len(faultList)-1) else 0)
                else: # include missing faults
                    if self.environment==environmentsE.gem5armv7.name or self.environment==environmentsE.gem5armv8.name:
                        temp =str(linecache.getline(self.faultlist,i+1).rstrip()+" "+errorAnalysis.RD_PRIV.name+" "+str(errorAnalysis.RD_PRIV.value)+" 0 "+instructionCount+" Null 0")
                        faultList.append(temp.split())
                        recoveredfaults+=1
                        faultList[-1][4] = faultList[-1][4].split(":")[0]
                    else:
                        temp =str(linecache.getline(self.faultlist,i+1).rstrip()+" "+errorAnalysis.Hang.name+" "+str(errorAnalysis.Hang.value)+" 0 "+instructionCount+" Null 0")
                        faultList.append(temp.split())
                        recoveredfaults+=1
                        faultList[-1][4] = faultList[-1][4].split(":")[0]


        # -------------------------------------- Making some calculation ------------------------------------ #
        # total number of REALLY executed instructions
        totalExecutedInstructions=int(sum([int(item[9]) for item in faultList]))
        mipsExecutedInstructions =float(totalExecutedInstructions)/float(self.exectime /1000.0)

        totalExecutedInstructionsWithChkp=0
        for fault in faultList:
            if(fault[9]=="0"):
                print(fault)
            totalExecutedInstructionsWithChkp += int(fault[11]) + int(fault[9])

        mipsExecutedInstructionsWithChkp =float(totalExecutedInstructionsWithChkp)/(self.exectime /1000.0)

        # -------------------------------------- Generate the Output File ----------------------------------- #
        outputfile=self.diroutput+self.application+"."+self.environment+".reportfile"
        fileptr = open(outputfile, 'w')

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
        fileptr.write("%s %s\n" %("Application".ljust(spacing),self.application))
        fileptr.write("%s %d\n" %("Recovered faults".ljust(spacing),self.numfaults))
        fileptr.write("%s %d\n" %("Target Cores".ljust(spacing),self.numcores))
        fileptr.write("%s %s\n" %("Environment".ljust(spacing),self.environment))
        fileptr.write("%s %s\n" %("CPU type".ljust(spacing),self.cputype))

        fileptr.write("%s %d\n" %("Executed Instructions".ljust(spacing),totalExecutedInstructions))
        fileptr.write("%s %d\n" %("Total Emulated".ljust(spacing),totalExecutedInstructionsWithChkp))
        fileptr.write("%s %d\n" %("Gold Executed Instructions".ljust(spacing),int(instructionCount)))

        fileptr.write("%s %.3f\n" %("Simulation Time (seconds)".ljust(spacing),self.exectime/1000000000.0))
        fileptr.write("%s %.3f\n" %("Million instructions per second (MIPS)".ljust(spacing),mipsExecutedInstructions))
        fileptr.write("%s %.3f\n" %("MIPS if with checkpoint".ljust(spacing),mipsExecutedInstructionsWithChkp))

        fileptr.close()
        
        return outputfile

