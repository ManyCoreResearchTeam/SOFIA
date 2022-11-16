#   Fault Injection Module Graphical Interface
#
#
#
#
#   Author: Felipe Rocha da Rosa

# Libraries
import sys

import subprocess
import ctypes
import copy
import re
from random     import randint
from random     import sample
from array      import *

import numpy as np
import os.path

# Local classes
# Remove incorrect include paths
if len([s for s in sys.path if "support" in s])!=0:
    sys.path.remove([s for s in sys.path if "support" in s][0])

from classification import *
from commondefines  import *

class faultGenerator:
    def __init__(self,options,faultlist="./faultlist",goldinformation="./goldinformation",tracefile="trace.log"):
        self.options         =  options
        self.faultlist       =  faultlist
        self.goldinformation =  goldinformation
        self.tracefile       =  tracefile

    def createfaultlist(self):
        ####################################################################################################
        # Collect the information                                                                          #
        ####################################################################################################
        # default
        numcores = 1

        # get the instruction count for each core
        coreinstcount = (str(subprocess.check_output('grep -i "Executed Instructions Per core=" %s'%self.goldinformation,shell=True,)).strip().split("=")[-1])
        coreinstcount = coreinstcount.split(",")
        # Number of cores
        numcores = len(coreinstcount)
        coreinstcount = [ int(re.sub("\D", "", coreinstcount[i])) for i in range(numcores) ]
        possibletargetcores = [ int(i) for i in range(numcores) if(coreinstcount[i] != 0) ]

        #Get the application entry point
        if not self.options.environment==environmentsE.gem5armv7.name and not self.options.environment==environmentsE.gem5armv8.name:
            # get the instruction count for each core where the application begin
            corebegin = (str(subprocess.check_output('grep -i "Application begins=" %s'%self.goldinformation,shell=True,)).strip().split("=")[-1])
            corebegin = corebegin.split(",")
        else: # the Gem5 always is zero because the checkpoint load befores the application reset
            corebegin = np.zeros(numcores)

        corebegin = [ int(re.sub("\D", "", corebegin[i])) for i in range(numcores) ]
        ####################################################################################################
        # register file                                                                                    #
        ####################################################################################################
        # create a fault to inject in one of the general purpose registers
        if self.options.faulttype == faultTypesE.register.name or self.options.faulttype == faultTypesE.functiontrace.name or self.options.faulttype == faultTypesE.functiontrace2.name:
            # list of possible registers
            archregisters = archRegisters()

            #Registers for each simulator environment
            targetregisters = archregisters.possibleregisters(self.options.environment)
            numregisters = archregisters.getNumberOfRegisters()
            numfaultsbyreg = np.zeros(numregisters)

            targetregistersRestrited=[]

            # list of registers informed
            if (self.options.registerlist is not None) and (len(self.options.registerlist)>0):
                for regi in self.options.registerlist.split(","):
                    targetregistersRestrited.append((targetregisters.index(regi),regi))
            else: #Use all available
                for regi in targetregisters:
                    targetregistersRestrited.append((targetregisters.index(regi),regi))


            # replace the old list
            targetregisters = copy.deepcopy(targetregistersRestrited)
            numregisters = len(targetregisters)

            if self.options.eqdist: # generate the same number of faults for each registers in the list
                if (self.options.numfaults % numregisters) == 0 :
                    faultPerRegister = self.options.numfaults / numregisters
                else:
                    print(("The number of faults are not equally divided by the number of possible target registers {0} !!!".format(numregisters)))
                    sys.exit(-1)

        ####################################################################################################
        # function trace                                                                                   #
        ####################################################################################################
        if self.options.faulttype==faultTypesE.functiontrace.name:
            # read the file
            functiontrace   = [line.rstrip('\n') for line in open(self.tracefile)]
            availableranges = []

            for l in functiontrace: # parse the input
                line=l.split(" ")
                if(line[1]=="ENTER"):  # entry point
                    availableranges.append({"Index":line[0],"Core":line[4],"Begin":line[6]})
                elif(line[1]=="EXIT"): # update the dictionary entry with the final ICOUNT
                    (item for item in availableranges if item["Index"] == line[0]).next().update({"End":line[6]})

            for item in availableranges:
                if not "EXIT" in item: # search for ranges without closing statement (i.e. the function never returns)
                    item.update({"End":coreinstcount[int(item["Core"])]})

        # Detailed trace
        if self.options.faulttype==faultTypesE.functiontrace2.name:
            # read the file
            availableranges   = [line.rstrip('\n') for line in open(self.tracefile)]

        ####################################################################################################
        # variable trace                                                                                   #
        ####################################################################################################
        if self.options.faulttype==faultTypesE.variable.name:
            # read the file
            variableTrace   = [line.rstrip('\n') for line in open(self.tracefile)]

        if self.options.faulttype==faultTypesE.variable2.name or self.options.faulttype==faultTypesE.functioncode.name :
            # read the file
            tracefile = [line.rstrip('\n').split() for line in open(self.tracefile)]
            self.options.memlowaddress =int(tracefile[0][0])
            self.options.memhighaddress=int(tracefile[0][1])

        ####################################################################################################
        # fault generation                                                                                 #
        ####################################################################################################
        # Create the fault file
        fileptr = open(self.faultlist,'w')

        # File header
        fileptr.write("%8s %7s %10s %2s %18s %7s\n" %("Index","Type","Target","i","Insertion Time","Mask"))

        # Counter of created faults per core
        targetcores = [0]*(numcores)

        # Number of faults
        for i in range(1,self.options.numfaults+1):
            #
            # Fault Type
            #
            faulttype = self.options.faulttype

            #
            # Fault Injection Time
            #
            if   self.options.faulttype == faultTypesE.functiontrace.name: # create fault from a list of ranges
                # select one block
                selectedRange = availableranges[randint(0,len(availableranges)-1)]

                # copy the values
                faultCore = int(selectedRange["Core"])
                faultTime = randint(int(selectedRange["Begin"]), int(selectedRange["End"]))

            elif self.options.faulttype == faultTypesE.functiontrace2.name: # create fault from a list of ranges for the detailed trace
                # select one block
                # Each line contains: Start inst End inst CPU id Symbol
                selectedRange = availableranges[randint(0,len(availableranges)-1)]

                # split in the space mark
                line       = selectedRange.split()
                startblock = int(line[0])
                endblock   = int(line[1])
                cpuid      = int(line[2])
                symbol     = line[3]

                # copy the values
                faultCore = cpuid
                faultTime = randint(int(startblock),int(endblock))

            elif self.options.faulttype == faultTypesE.variable.name: # create fault from the number of transactions
                faultCore = 0 # in this context the core is not used

            else: # create from a flat range
                # choose one core that is not has a zero instruction count
                coreIndex = randint(0,len(possibletargetcores)-1)
                faultCore = possibletargetcores[coreIndex]
                targetcores[faultCore]+=1


                # retrieve cores with enough faults
                if self.options.eqdist and (targetcores[faultCore] == (self.options.numfaults / numcores)):
                    possibletargetcores.remove(faultCore)

                # insertion time -- each core has a different instruction count
                faultTime = randint(0, int(int(coreinstcount[faultCore]) - int(corebegin[faultCore])) ) + int(corebegin[faultCore])

            #
            # Fault target and mask for each fault type
            #
            masks = []
            shiftValues = []
            # register fault
            if   ([ True for item in registerfaultsL if item==self.options.faulttype]):
                # choose the register
                faultRegisterIndex = randint(0,len(targetregisters)-1)
                ##print(len(targetregisters))
                numfaultsbyreg[int(targetregisters[faultRegisterIndex][0])]+=1

                numFlips = self.options.bitFlips
                # ARMv8 64bit register
                if self.options.environment==environmentsE.ovparmv8.name or self.options.environment==environmentsE.gem5armv8.name:
                    if self.options.dummy:
                        faultMask = ctypes.c_uint64(0xFFFFFFFFFFFFFFFF)         #No effect
                    elif targetregisters[faultRegisterIndex][1]=="pc":
                        if self.options.sequentialBits:
                            maxN = 57 - numFlips
                            shift = randint(0,maxN)
                            for x in range(shift, shift+numFlips): 
                                shiftValues.append(x)
                        else:
                            shiftValues = sample(range(0,57), numFlips)
                        for j in shiftValues:
                            masks.append(ctypes.c_uint64(~(0x10 << j)))
                        faultMask = masks[0]
                        for j in range (1, numFlips):
                            faultMask = ctypes.c_uint64(faultMask.value & masks[j].value)
                    else:
                        if self.options.sequentialBits:
                            maxN = 63 - numFlips
                            shift = randint(0,maxN)
                            for x in range(shift, shift+numFlips): 
                                shiftValues.append(x)
                        else:
                            shiftValues = sample(range(0,63), numFlips)
                        for j in shiftValues:
                            masks.append(ctypes.c_uint64(~(0x1 << j)))
                        faultMask = masks[0]
                        for j in range (1, numFlips):
                            faultMask = ctypes.c_uint64(faultMask.value & masks[j].value)
                else:
                    if self.options.dummy:
                        if targetregisters[faultRegisterIndex][1] in archregisters.floatRegisters:
                            faultMask = ctypes.c_uint64 (0xFFFFFFFFFFFFFFFF)    #Random mask of one bit between zero and 31 ones
                        else:
                            faultMask = ctypes.c_uint32(0xFFFFFFFF)                 #No effect
                    elif targetregisters[faultRegisterIndex][1]=="pc" or self.options.environment==environmentsE.riscv.name:
                        if self.options.sequentialBits:
                            maxN = 23 - numFlips
                            shift = randint(0,maxN)
                            for x in range(shift, shift+numFlips): 
                                shiftValues.append(x)
                        else:
                            shiftValues = sample(range(0,23), numFlips)
                        for j in shiftValues:
                            masks.append(ctypes.c_uint32(~(0x10 << j)))
                        faultMask = masks[0]
                        for j in range (1, numFlips):
                            faultMask = ctypes.c_uint32(faultMask.value & masks[j].value)
                    elif targetregisters[faultRegisterIndex][1] in archregisters.floatRegisters:
                        if self.options.sequentialBits:
                            maxN = 63 - numFlips
                            shift = randint(0,maxN)
                            for x in range(shift, shift+numFlips): 
                                shiftValues.append(x)
                        else:
                            shiftValues = sample(range(0,63), numFlips)
                        for j in shiftValues:
                            masks.append(ctypes.c_uint64(~(0x1 << j)))
                        faultMask = masks[0]
                        for j in range (1, numFlips):
                            faultMask = ctypes.c_uint64(faultMask.value & masks[j].value)
                    else:
                        if self.options.sequentialBits:
                            maxN = 31 - numFlips
                            shift = randint(0,maxN)
                            for x in range(shift, shift+numFlips): 
                                shiftValues.append(x)
                        else:
                            shiftValues = sample(range(0,31), numFlips)
                        for j in shiftValues:
                            masks.append(ctypes.c_uint32(~(0x1 << j)))
                        faultMask = masks[0]
                        for j in range (1, numFlips):
                            faultMask = ctypes.c_uint32(faultMask.value & masks[j].value)

                target=targetregisters[faultRegisterIndex][1]
                index=targetregisters[faultRegisterIndex][0]

                if self.options.eqdist and numfaultsbyreg[int(targetregisters[faultRegisterIndex][0])]==faultPerRegister:
                    targetregisters.remove(targetregisters[faultRegisterIndex])
                    numregisters = len(targetregisters)

            # reorder buffer source
            elif self.options.faulttype == faultTypesE.robsource.name:
                if self.options.dummy:
                    faultMask = ctypes.c_uint16(0xFFFF)                     # no effect in the register
                else:
                    faultMask = ctypes.c_uint16(~(0x1<< randint(0,15)))     # random mask of one bit between 0 and 15

                target="ROB"
                index="1"

            # reorder buffer destination
            elif self.options.faulttype == faultTypesE.robdest.name:
                if self.options.dummy:
                    faultMask = ctypes.c_uint16(0xFFFF)                     # no effect in the register
                else:
                    faultMask = ctypes.c_uint16(~(0x1<< randint(0,15)))     # random mask of one bit between 0 and 15

                target="ROB"
                index="2"

            # variable
            elif self.options.faulttype == faultTypesE.variable.name:
                selTransaction = randint(0, len(variableTrace))         # choose one transaction
                transaction    = variableTrace[selTransaction].split()  # split the data

                target    = transaction[1]                              # read or write transaction
                index     = int(randint(0,63))                          # the byte of the application to flip GET SIZE
                faultTime = int(transaction[0])                         # transaction index
                faultCore = 0                                           # in this context the core is not used
                target    = variableTrace[selTransaction].split()[1]    #

                # random mask of one bit in a four byte
                if self.options.dummy:
                    faultMask = ctypes.c_uint32(0xFFFFFFFF)                # no effect in the register
                else:
                    faultMask = ctypes.c_uint32(~(0x1<< randint(0,31)))    # random mask of one bit between zero and 31 ones

            # memory (flat range)
            elif ([ True for item in memoryfaultsL if item==self.options.faulttype]):
                index="0"
                target=randint(self.options.memlowaddress,self.options.memhighaddress)
                #Addressable memory range
                if self.options.environment==environmentsE.ovparmv8.name or self.options.environment==environmentsE.gem5armv8.name:
                    target="%016X"%(randint(self.options.memlowaddress,self.options.memhighaddress))
                else:
                    target="%08X"%(randint(self.options.memlowaddress,self.options.memhighaddress))

                # random mask of one bit in a four byte
                if self.options.dummy:
                    faultMask = ctypes.c_uint8(0xFF)                     # no effect
                else:
                    faultMask = ctypes.c_uint8(~(0x1<< randint(0,7)))    # random mask of one bit between zero and 31 ones

            fileptr.write("%7d %10s %16s %4s %15d:%d %12X\n" % (i,faulttype,target,index,faultTime,faultCore,faultMask.value))
        fileptr.close()

        return self.faultlist
