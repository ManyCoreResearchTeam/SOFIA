#!/usr/bin/python

#My imports
import sys

import subprocess
import linecache
import ctypes
import copy
import re
from optparse   import OptionParser
from random     import randint
from array      import *
import pylab as P
import numpy as np
from collections import namedtuple

#python common class
# -> see classication.py
from classification import *

# -------------- Fault Campaign options -------------- #
usage = "usage: %prog [options] arg1 arg2" # @Review
parser = OptionParser(usage=usage)

# options
parser.add_option("--numberoffaults",  action="store", type="int",     dest="numberoffaults",  help="Number of faults to be created", default=1)
parser.add_option("--reglist",         action="store", type="string",  dest="reglist",         help="List of target registers separeted by comma ','")
parser.add_option("--faultlist",       action="store", type="string",  dest="faultlist",       help="File containing the fault list [default: %default]",           default="./faultlist")
parser.add_option("--goldinformation", action="store", type="string",  dest="goldinformation", help="File containing the reference information [default: %default]",default="./goldinformation")
parser.add_option("--ftracefile",      action="store", type="string",  dest="ftracefile",      help="File containing the function trace dump [default: %default]",  default="./TRACE_FUNCTION")
parser.add_option("--vtracefile",      action="store", type="string",  dest="vtracefile",      help="File containing the variable trace dump [default: %default]",  default="./TRACE_VARIABLE")

# architecture
parser.add_option("--environment", type="choice", dest="environment", choices=architectures, help="Choose one architecture mode", default=architectures[0])

# fault type
parser.add_option("--faulttype",   type="choice", dest="faulttype", choices=faultTypes, help="Choose one fault injection type", default=faultTypes[0])

# other options
parser.add_option("--nobitflip", action="store_true", dest="nobitflip", help="Create faults that DO NOT generate errors (i.e. any bit will be flipped)")
parser.add_option("--eqdist",    action="store_true", dest="eqdist",    help="The number of faults will be evenly distributed among the registers and cores available")

# memory fault options
parser.add_option("--memaddress",action="store", type="long", dest="memaddress", help="Default base memory address",default=0x0)
parser.add_option("--memsize",   action="store", type="long", dest="memsize",    help="Default base memory size (in hex)",default=0x8000000)

# parse the options
(options, args) = parser.parse_args()

####################################################################################################
# Options Checker                                                                                  #
####################################################################################################


####################################################################################################
# Collect the information                                                                          #
####################################################################################################
# default
numberOfCores = 1

# get the instruction count for each core
individualCoreGoldCount = (subprocess.check_output('grep "Executed Instructions Per core=" %s'%options.goldinformation,shell=True,).strip().split("=")[-1])
individualCoreGoldCount = individualCoreGoldCount.split(",")
numberOfCores = len(individualCoreGoldCount)
listPossibleCores = [ int(i) for i in range(numberOfCores) if(int(individualCoreGoldCount[i])!= 0) ]

#Get the application entry point
if not options.environment==archtecturesE.gem5armv7.name and not options.environment==archtecturesE.gem5armv8.name:
    # get the instruction count for each core where the application begin
    individualCoreAppBegan = (subprocess.check_output('grep "Application begins=" %s'%options.goldinformation,shell=True,).strip().split("=")[-1])
    individualCoreAppBegan = individualCoreAppBegan.split(",")
else: # the Gem5 always is zero because the checkpoint load before the application resets the instruction counter
    individualCoreAppBegan = np.zeros(numberOfCores)

####################################################################################################
# register file                                                                                    #
####################################################################################################

# create a fault to inject in one of the general purpose registers
if options.faulttype == faultTypesE.register.name or options.faulttype == faultTypesE.functiontrace.name:
    # list of possible registers
    archRegisters = archRegisters()

    #Registers for each simulator environment
    possibleRegisters = archRegisters.getListOfpossibleRegisters(options,parser)
    numberOfRegisters = archRegisters.getNumberOfRegisters()
    numberOfFaultsPerReg = np.zeros(numberOfRegisters)

    possibleRegistersRestrited=[]

    # list of registers informed
    if (options.reglist is not None) and (len(options.reglist)>0):
        for regi in options.reglist.split(","):
            possibleRegistersRestrited.append((possibleRegisters.index(regi),regi))
    else: #Use all available
        for regi in possibleRegisters:
            possibleRegistersRestrited.append((possibleRegisters.index(regi),regi))

    # replace the old list
    possibleRegisters = copy.deepcopy(possibleRegistersRestrited)
    numberOfRegisters = len(possibleRegisters)

    if options.eqdist: # generate the same number of faults for each registers in the list
        if (options.numberoffaults % numberOfRegisters) == 0 :
            faultPerRegister = options.numberoffaults / numberOfRegisters
        else:
            print("The number of faults are not equally divided by the number of possible target registers {0} !!!".format(numberOfRegisters))
            sys.exit(-1)

####################################################################################################
# function trace                                                                                   #
####################################################################################################
if options.faulttype==faultTypesE.functiontrace.name:
    # read the file
    functionTrace   = [line.rstrip('\n') for line in open(options.ftracefile)]
    availableRanges = []

    for l in functionTrace: # parse the input
        line=l.split(" ")
        if(line[1]=="ENTER"):  # entry point
            availableRanges.append({"Index":line[0],"Core":line[4],"Begin":line[6]})
        elif(line[1]=="EXIT"): # update the dictionary entry with the final ICOUNT
            (item for item in availableRanges if item["Index"] == line[0]).next().update({"End":line[6]})

    for item in availableRanges:
        if not "EXIT" in item: # search for ranges without closing statement (i.e. the function never returns)
            item.update({"End":individualCoreGoldCount[int(item["Core"])]})


####################################################################################################
# variable trace                                                                                   #
####################################################################################################
if options.faulttype==faultTypesE.variable.name:
    # read the file
    variableTrace   = [line.rstrip('\n') for line in open(options.vtracefile)]

####################################################################################################
# fault generation                                                                                 #
####################################################################################################
#Create the file
fileptr = open(options.faultlist,'w')

#File header
fileptr.write("%8s %7s %10s %2s %18s %7s\n" %("Index","Type","Target","i","Insertion Time","Mask"))

#Counter of created faults per core
possibleTargetCores = [0]*(numberOfCores)

for i in range(1,options.numberoffaults+1):
    # fault injection time
    if options.faulttype == faultTypesE.functiontrace.name: # create fault from a list of ranges
        # select range
        selectedRange = availableRanges[randint(0,len(availableRanges)-1)]

        # copy the values
        faultCore = int(selectedRange["Core"])
        faultTime = randint(int(selectedRange["Begin"]), int(selectedRange["End"]))
    elif options.faulttype == faultTypesE.variable.name: # create fault from the number of transactions
        faultCore = 0 # in this context the core is not used
    else: # create from a flat range
        # choose one core that is not has a zero instruction count
        coreIndex = randint(0,len(listPossibleCores)-1)
        faultCore = listPossibleCores[coreIndex]
        possibleTargetCores[faultCore]+=1

        # retrieve cores with enough faults
        if options.eqdist and (possibleTargetCores[faultCore] == (options.numberoffaults / numberOfCores)):
            listPossibleCores.remove(faultCore)

        # insertion time -- each core has a different instruction count
        faultTime = randint(0, int(int(individualCoreGoldCount[faultCore]) - int(individualCoreAppBegan[faultCore])) ) + int(individualCoreAppBegan[faultCore])

    # fault type
    faulttype = options.faulttype

    #fault targer and mask by fault type
    # register fault
    if options.faulttype == faultTypesE.register.name or options.faulttype == faultTypesE.functiontrace.name:
        # choose the register
        faultRegisterIndex = randint(0,len(possibleRegisters)-1)
        numberOfFaultsPerReg[int(possibleRegisters[faultRegisterIndex][0])]+=1

        if options.environment==archtecturesE.ovparmv8.name or options.environment==archtecturesE.gem5armv8.name:
            if options.nobitflip:
                faultMask = ctypes.c_uint64(0xFFFFFFFFFFFFFFFF)         #No effect
            elif possibleRegisters[faultRegisterIndex][1]=="pc":
                faultMask = ctypes.c_uint64 (~(0x10<< randint(0,57)))   #Random mask of one bit between 4 and 31 ones
            else:
                faultMask = ctypes.c_uint64 (~(0x1<< randint(0,61)))    #Random mask of one bit between zero and 31 ones
        else:
            if options.nobitflip:
                faultMask = ctypes.c_uint32(0xFFFFFFFF)                 #No effect
            elif possibleRegisters[faultRegisterIndex][1]=="pc":
                faultMask = ctypes.c_uint32 (~(0x10<< randint(0,23)))   #Random mask of one bit between 4 and 31 ones
            else:
                faultMask = ctypes.c_uint32 (~(0x1<< randint(0,31)))    #Random mask of one bit between zero and 31 ones

        target=possibleRegisters[faultRegisterIndex][1]
        index=possibleRegisters[faultRegisterIndex][0]

        # retrieve register
        if options.eqdist and numberOfFaultsPerReg[int(possibleRegisters[faultRegisterIndex][0])]==faultPerRegister:
            possibleRegisters.remove(possibleRegisters[faultRegisterIndex])
            numberOfRegisters = len(possibleRegisters)

    # reorder buffer source
    elif options.faulttype == faultTypesE.robsource.name:
        if options.nobitflip:
            faultMask = ctypes.c_uint16(0xFFFF)                     # no effect in the register
        else:
            faultMask = ctypes.c_uint16(~(0x1<< randint(0,15)))     # random mask of one bit between 0 and 15

        target="ROB"
        index="1"

    # reorder buffer destination
    elif options.faulttype == faultTypesE.robdest.name:
        if options.nobitflip:
            faultMask = ctypes.c_uint16(0xFFFF)                     # no effect in the register
        else:
            faultMask = ctypes.c_uint16(~(0x1<< randint(0,15)))     # random mask of one bit between 0 and 15

        target="ROB"
        index="2"

    # variable
    elif options.faulttype == faultTypesE.variable.name:
        selTransaction = randint(0, len(variableTrace))         # choose one transaction
        transaction    = variableTrace[selTransaction].split()  # split the data

        target    = transaction[1]                              # read or write transaction
        index     = int(randint(0,63))                          # the byte of the application to flip GET SIZE
        faultTime = int(transaction[0])                         # transaction index
        faultCore = 0                                           # in this context the core is not used
        target    = variableTrace[selTransaction].split()[1]    #

        # random mask of one bit in a four byte
        if options.nobitflip:
            faultMask = ctypes.c_uint32(0xFFFFFFFF)                 # no effect in the register
        else:
            faultMask = ctypes.c_uint32(~(0x1<< randint(0,31)))    # random mask of one bit between zero and 31 ones

    # memory (flat range)
    elif options.faulttype == faultTypesE.memory.name:
        index="0"
        target=randint(options.memaddress,options.memsize)
        #Addressable memory range
        if options.environment==archtecturesE.ovparmv8.name or options.environment==archtecturesE.gem5armv8.name:
            target="%016X"%(randint(options.memaddress,options.memsize))
        else:
            target="%08X"%(randint(options.memaddress,options.memsize))

        # random mask of one bit in a four byte
        if options.nobitflip:
            faultMask = ctypes.c_uint8(0xFF)                   # no effect in the register
        else:
            faultMask = ctypes.c_uint8(~(0x1<< randint(0,7)))    # random mask of one bit between zero and 31 ones



    fileptr.write("%7d %10s %16s %4s %15d:%d %12X\n" % (i,faulttype,target,index,faultTime,faultCore,faultMask.value))
fileptr.close()
