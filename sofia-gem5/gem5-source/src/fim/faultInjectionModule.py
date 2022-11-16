"""
Fault Injection Module to Gem5
Version 1.0
02/02/2016

Author: Felipe Rocha da Rosa - frdarosa.com
"""
import sys
from os import getcwd
from os.path import join as joinpath

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.util import *
from m5.util import addToPath, fatal

addToPath('../../../')
addToPath('../../config/common')
addToPath('../../config/ruby')
addToPath('../../../support/tools')

from common import MemConfig
from common import CpuConfig

#My imports
from array import *
from enum import Enum
import subprocess
import linecache
import ctypes
import numpy as np
from operator import itemgetter

#python common class
# -> see classication.py
from classification import *

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

class FIM:
    """Fault injection class"""
    platformIdentifier = 0                                  # platform unique identifier
    faultIndex = 0                                          # the current fault to be injected
    faultList  = []                                         # list of faults to be injected

    #Possible register list
    possibleRegisters = ["r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12","r13","r14","pc"]

    #Number of registers
    numberOfRegisters = len(possibleRegisters)-1

    """Class Creator"""
    def __init__(self):
        self.linuxBootInstructions=0
        self.hangOverRun=1.5

    """Initialize the module"""
    def FIMcreation(self,options,system,cptdir):
        self.options=options
        self.system=system
        self.platformIdentifier = self.options.platformid
        self.cptdir = cptdir
        self.instructionsGoldMax = np.zeros(options.num_cpus)
        self.correctCheckpoint   = np.zeros(options.num_cpus)
        self.faultTypes = faultTypes                                 #Fault types

        #Output Folders
        self.applicationCurrentFolder = self.options.applicationfolder
        self.applicationOutputFolder  = self.cptdir
        self.goldFolder               = "{0}/m5outfiles/gold".format(self.applicationCurrentFolder)

        #verify arguments compatibility
        if self.options.faultcampaignrun and self.options.goldrun:
            fatal("Can't specify both --faultcampaignrun --goldrun")
            sys.exit(1)

        #Gold Run
        if options.goldrun:
            self.state = simulationStatesE.FIM_GOLD_EXECUTION
            self.faultCoreToInject = 0

        #Acquire the list of fault to this platform
        if options.faultcampaignrun:
            self.state = simulationStatesE.FIM_BEFORE_INJECTION

            #Retrieve the information about the gold execution
            self.retrieveInformation(self.applicationCurrentFolder)

            # If this maxinsts was triggered during the fault injection means that the applications reached more than 50% the
            # normal value extract from the fault free execution. To classification purpose, will be considered that the
            # application is in a infinite loop. Each core will have a distinct value?
            for i in range(options.num_cpus):
                self.instructionsGoldMax[i]=int(int(self.perCoreGoldCount[i])*self.hangOverRun)

            #determine the correct line of the file
            linePosition = ((self.platformIdentifier-1)*self.options.numberoffaults)+1;

            #Collect the faults (one per default)
            for i in range(1,self.options.numberoffaults+1):
                self.faultList.append(linecache.getline("{0}/{1}".format(self.applicationCurrentFolder,self.options.faultlist), linePosition+i).split())

            #Core to inject
            self.faultCoreToInject=int(self.faultList[0][4].split(":")[-1])

            #Remove the core value and the :
            self.faultList[0][4]=self.faultList[0][4].split(":")[0]

        # Define the CPU number according with the model
        if  (self.options.environment=="gem5armv7"):
                if  (self.options.cpu_type=="atomic"):
                    self.cpuTypeObj=0
                elif(self.options.cpu_type=="timing"):
                    self.cpuTypeObj=1
                elif(self.options.cpu_type=="detailed"):
                    self.cpuTypeObj=2
        elif(self.options.environment=="gem5armv8"):
                if  (self.options.cpu_type=="atomic"):
                    self.cpuTypeObj=3
                elif(self.options.cpu_type=="timing"):
                    self.cpuTypeObj=4
                elif(self.options.cpu_type=="detailed"):
                    self.cpuTypeObj=5

        #Create the C++ object to interact during the simulation
        self.system.faultSystem = faultInjectionSystem(system=self.system.cpu[self.faultCoreToInject],cpuType=self.cpuTypeObj,mem=self.system.mem_ctrls[0])

    """Add the FIModule options to the parser"""
    def optionsParser(self,parser):
        parser.add_option("--goldrun",                  action="store_true", help="Gold Execution")
        parser.add_option("--fullsystem",               action="store_true", help="Full System")
        parser.add_option("--kernelcheckpointafterboot",action="store_true", help="Create a checkpoint at the end of the linux boot")
        parser.add_option("--checkpointswitch",         action="store_true", help="Create multiple checkpoints using the detailed mode")
        parser.add_option("--faultcampaignrun",         action="store_true", help="Fault Campaign Execution")
        parser.add_option("--numberoffaults",           action="store",      type="int",     dest="numberoffaults",     help="Number of faults to be injected",                         default=1)
        parser.add_option("--checkpointinterval",       action="store",      type="int",     dest="checkpointinterval", help="Create periodical checkpoints during the gold execution", default=0)
        parser.add_option("--platformid",               action="store",      type="int",     dest="platformid",         help="Unique platform identifier",                              default=0)
        parser.add_option("--faultlist",                action="store",      type="string",  dest="faultlist",          help="File containing the fault list",                          default="faultlist")
        parser.add_option("--kernelcheckpoint",         action="store",      type="string",  dest="kernelcheckpoint",   help="Checkpoint for full system")
        parser.add_option("--projectfolder",            action="store",      type="string",  dest="projectfolder",      help="Project folder")
        parser.add_option("--applicationfolder",        action="store",      type="string",  dest="applicationfolder",  help="Application folder")
        parser.add_option("--environment",              action="store",      type="string",  dest="environment",        help="Environment variable")
        parser.add_option("--goldinformation",          action="store",      type="string",  dest="goldinformation",    help="File containing the fault list [default: %default]",      default="goldinformation")

    """Set the next fault """
    def setFault(self,faultNumber):
        self.faultIndex             = self.faultList[faultNumber][0]
        self.faultType              = self.faultList[faultNumber][1]
        self.faultRegister          = self.faultList[faultNumber][2]
        self.faultRegisterIndex     = self.faultList[faultNumber][3]
        self.faultTime              = self.faultList[faultNumber][4]
        self.faultMask              = self.faultList[faultNumber][5]

        if self.faultRegister=="pc":
            targetPC=True
        else:
            targetPC=False

        #Inject the fault using the C++ object int faultMask, int faultType, int faultRegister, bool targetPC
        # in the fault list is called target not fault register
        if self.faultType==faultTypesE.register.name:
            self.system.faultSystem.setupFault(ctypes.c_int64(int(self.faultMask,16)).value, faultTypesE.register.value,  int(self.faultRegisterIndex), targetPC, 0x0)
        elif self.faultType==faultTypesE.robsource.name:
            self.system.faultSystem.setupFault(ctypes.c_int64(int(self.faultMask,16)).value, faultTypesE.robsource.value, int(self.faultRegisterIndex), False,    0x0)
        elif self.faultType==faultTypesE.robdest.name:
            self.system.faultSystem.setupFault(ctypes.c_int64(int(self.faultMask,16)).value, faultTypesE.robdest.value,   int(self.faultRegisterIndex), False,    0x0)
        elif self.faultType==faultTypesE.memory.name:
            self.system.faultSystem.setupFault(ctypes.c_int64(int(self.faultMask,16)).value, faultTypesE.memory.value,    int(self.faultRegisterIndex), False,    ctypes.c_uint64(int(self.faultRegister,16)).value) # in the faultlist is called target not fault register

        #Required after m5.instantiate(checkpointDir)! --Fault time should be adjusted to use checkpoint and -1
        self.system.cpu[self.faultCoreToInject].scheduleInstStop(0,int(int(self.faultTime)-int(self.correctCheckpoint[self.faultCoreToInject])-1), simulationStatesE.FIM_BEFORE_INJECTION.name)

    """Insert the fault into the processor"""
    def injectFault(self):
        #Verify the actual state of the fault simulation
        if self.state == simulationStatesE.FIM_BEFORE_INJECTION:
            # Inject Faults
            self.system.faultSystem.faultInjectionHandler()

            #Adjust the scheduler to trigger the next state
            for i in range(self.options.num_cpus):
                if self.instructionsGoldMax[i]!=0:
                    stopPoint = self.instructionsGoldMax[i] - self.system.cpu[i]._ccObject.totalInsts()
                    self.system.cpu[i].scheduleInstStop(0,int(stopPoint),simulationStatesE.FIM_WAIT_HANG.name)

            #Next state
            self.state = simulationStatesE.FIM_WAIT_HANG
            return True

    """Create checkpoint in a interval"""
    def setProfilingGold(self):
        self.gap = self.options.checkpointinterval - (self.options.checkpointinterval%1000000)
        print "GAP ",self.gap,self.options.checkpointinterval,(self.options.checkpointinterval%1000000)

        #Schedule the next checkpoint
        self.system.cpu[0].scheduleInstStop(0,int(self.gap)-1,simulationStatesE.FIM_GOLD_PROFILING.name)

    """Creates the checkpoint during the simulation"""
    def profilingGold(self,exitCause):
        #Profile the application
        if exitCause==simulationStatesE.FIM_GOLD_PROFILING.name or self.exitCause == "simulate() limit reached":
            print "FIM: Checkpoint at %d" %self.totalSimulatedInst()
            #~ m5.drain()

            #Schedule the next checkpoint
            self.system.cpu[0].scheduleInstStop(0,int(self.gap),simulationStatesE.FIM_GOLD_PROFILING.name)

            #Checkpoint name
            checkpointName="FIM.Profile."+str(self.system.cpu[0]._ccObject.totalInsts())
            for i in range(1,self.options.num_cpus):
                checkpointName+="."+str(self.system.cpu[i]._ccObject.totalInsts())
            print checkpointName

            #Create checkpoint
            m5.checkpoint(joinpath(self.cptdir,checkpointName),True)

            #~ newcheckpointName="FIM.Profile."+str(self.system.cpu[0]._ccObject.totalInsts())
            #~ for i in range(1,self.options.num_cpus):
                #~ newcheckpointName+="."+str(self.system.cpu[i]._ccObject.totalInsts())
            #~ print newcheckpointName

            #~ subprocess.call("mv {0} {1}".format(joinpath(self.cptdir,checkpointName),joinpath(self.cptdir,newcheckpointName)), shell=True)

        #checkpoint message coming from the linux --The first will be just after the linux boot
        if exitCause=="checkpoint":
            print "FIM: kernel boot at %d" %self.totalSimulatedInst()

            #Create checkpoint
            m5.checkpoint(joinpath(self.cptdir,"FIM.After_Kernel_Boot.%d"%(self.totalSimulatedInst())))

            #Number of instructions to boot
            self.linuxBootInstructions = int(self.totalSimulatedInst())

            #Exit here - checkpoint ok
            sys.exit()

    """ Test if the memory was correct after compare with the gold run"""
    def testMemory(self):
        memInconsistency = False

        #Select the gold run memory to compare
        #Check if a checkpoint was reloaded
        if self.checkpointReloaded==False:
            path="{0}/gold_execution".format(self.goldFolder)
        else:
            self.instructionsGold=int(self.instructionsGold)
            path=self.checkpointFile.replace("FIM","END")
            #~ #Each checkpoint in the detailed mode has a memory
            #~ if self.options.cpu_type=="detailed":
                #~ self.instructionsGold=int(self.instructionsGold)
                #~ path=self.checkpointFile.replace("FIM","END")
            #~ else:
                #~ path="{0}/gold_execution".format(self.goldFolder)
                #~ # path="{0}/m5out/gold_execution_checkpoint".format(self.applicationCurrentFolder)

        #~ print path
        #~ print self.applicationOutputFolder

        #Get the number of memories
        NumberOfMemories =int(subprocess.check_output('find {0} -name "system.physmem.store*" | wc -l'.format(path),shell=True,))

        #Verify the consistency of the memory and caches
        for i in range(NumberOfMemories):
            compareMemCmd="cmp {0}/FIM.results.{1}.{2}/system.physmem.store{3}.pmem {4}/system.physmem.store{3}.pmem"\
            .format(self.applicationOutputFolder,self.options.platformid,int(self.faultIndex),i,path)
            print compareMemCmd
            try:
                memInconsistency = subprocess.check_output(compareMemCmd,shell=True,)
            except subprocess.CalledProcessError as e:
                print bcolors.WARNING+"ERROR %d!!!!!!!!"%i+bcolors.ENDC
                memInconsistency = True

        #Remove files used in this comparison
        subprocess.check_output("rm -rf {0}/FIM.results.{1}.{2}".format(self.applicationOutputFolder,self.options.platformid,int(self.faultIndex)), shell=True)
        #~ subprocess.check_output("rm -rf {0}".format(self.applicationOutputFolder), shell=True)

        if memInconsistency:
            print bcolors.WARNING+"ERROR !!!!!!!!"+bcolors.ENDC

        #In expecial cases... review
        if self.options.cpu_type=="atomic" and self.checkpointReloaded and memInconsistency and self.options.num_cpus>2:
            memInconsistency = False
            path="{0}/gold/gold_execution_checkpoint".format(self.applicationOutputFolder)
            for i in range(NumberOfMemories):
                compareMemCmd="cmp {0}/FIM.results.{1}.{2}/system.physmem.store{3}.pmem {4}/system.physmem.store{3}.pmem"\
                .format(self.applicationOutputFolder,self.options.platformid,int(self.faultIndex),i,path)
                print compareMemCmd

                try:
                    memInconsistency = subprocess.check_output(compareMemCmd,shell=True,)
                except subprocess.CalledProcessError as e:
                    memInconsistency = True

        if memInconsistency:
            Self.memInconsistency = "YES"
            print bcolors.WARNING+"FIM: Memory inconsistency"+bcolors.ENDC
        else:
            Self.memInconsistency = "NO"

        return memInconsistency

    """Finds the best checkpoint for this fault"""
    def checkpointDirectory(self):
        #Departure point checkpoint
        if self.options.fullsystem and not self.options.kernelcheckpointafterboot and len(self.options.kernelcheckpoint):
            #Load the checkpoint after the linux boot
            self.checkpointFile = subprocess.check_output('find %s -name "FIM.After_Kernel_Boot*"'%(self.options.kernelcheckpoint),shell=True,).rstrip()
        else:
            #No checkpoint necessary for the baremetal
            self.checkpointFile = None

        #A checkpoint in the middle of the application was reloaded? default false
        self.checkpointReloaded = False

        """During the Fault Campaign others checkpoints could be reloaded"""
        if self.options.faultcampaignrun and self.options.checkpointinterval!=0:
            #get all possible checkpoints
            getcheckpointFiles = subprocess.check_output('find . -name "FIM.Profile.*"',shell=True,).split()

            #Get the numbers
            getcheckpointFiles = [item.split(".") for item in getcheckpointFiles]

            #Sort the list according the faultCoreToInject
            getcheckpointFiles = sorted(getcheckpointFiles,key=lambda i: (int(i[self.faultCoreToInject+3])))

            #Checkpoints that could be restore in this case
            #self.faultList[0][4] --> Fault Position / faultCoreToInject+3 OBS:the +3 is due the file name after the split ./FIM.Profile.XXXX
            possibleCheckpoint=[int(item[self.faultCoreToInject+3]) for item in getcheckpointFiles if int(item[self.faultCoreToInject+3]) < int(self.faultList[0][4])]

            """A checkpoint in the middle of the application was chosen"""
            if len(possibleCheckpoint)!=0:
                #Select the best checkpoint if is the case
                Checkpoint=possibleCheckpoint.index(possibleCheckpoint[-1])

                #OBS:the +3 is due the file name after the split ./FIM.Profile.XXXX
                del getcheckpointFiles[Checkpoint][0]
                del getcheckpointFiles[Checkpoint][0]
                del getcheckpointFiles[Checkpoint][0]

                self.checkpointReloaded = True
                #Checkpoint name
                checkpointName="FIM.Profile"
                for i in range(self.options.num_cpus):
                    checkpointName+="."+getcheckpointFiles[Checkpoint][i]
                    self.correctCheckpoint[i]=int(getcheckpointFiles[Checkpoint][i])

                self.checkpointFile=joinpath(self.cptdir,"%s/%s"%(self.goldFolder,checkpointName))

                """Retrieve the information according with the correct checkpoint"""
                self.retrieveInformation(self.checkpointFile)

                # If this maxinsts was triggered during the fault injection means that the applications reached more than 50% the
                # normal value extract from the fault free execution. To classification purpose, will be considered that the
                # application is in a infinite loop (hang)
                for i in range(0,self.options.num_cpus):
                    self.perCoreGoldCount[i]=int(self.perCoreGoldCount[i]) - self.correctCheckpoint[i]
                    self.instructionsGoldMax[i]=int(self.perCoreGoldCount[i]*self.hangOverRun)

        """During the extend checkpoint creation to detailed (and atomic) cpu"""
        if self.options.goldrun and self.options.platformid!= 0:
            #get possible checkpoints
            getcheckpointFiles = subprocess.check_output('find . -name "FIM.Profile.*"',shell=True,).split()
            self.checkpointFile=getcheckpointFiles[int(self.options.platformid)-1].split("./")[-1]

            #Get the values of instruction where this platform will be loaded
            checkpointValue=self.checkpointFile.split(".")
            del checkpointValue[0]
            del checkpointValue[0]

            for i in range(self.options.num_cpus):
                self.correctCheckpoint[i]=int(checkpointValue[i])

            self.checkpointFile="%s/%s" % (self.applicationCurrentFolder,self.checkpointFile)
        return self.checkpointFile

    """Retrieve the gold information from the appropriate folder"""
    def retrieveInformation(self,path):
        path=path+"/"+self.options.goldinformation
        self.instructionsGold      = subprocess.check_output('grep "Application # of instructions" %s | cut -d \'=\' -f2'%(path),shell=True,).rstrip()
        self.linuxBootInstructions = subprocess.check_output('grep "Kernel Boot # of instructions" %s | cut -d \'=\' -f2'%(path),shell=True,).rstrip()
        self.TicksGold             = subprocess.check_output('grep "Number of Ticks" %s | cut -d \'=\' -f2'%(path),shell=True,).rstrip()
        self.perCoreGoldCount      = subprocess.check_output('grep "Executed Instructions Per core=" %s | cut -d \'=\' -f2'%(path),shell=True,).rstrip().split(",")

        self.FloatRegsGold         = subprocess.check_output('grep "Float Registers" %s | cut -d \'=\' -f2'%(path),shell=True,).rstrip()
        self.IntegerRegsGold       = subprocess.check_output('grep "Integer Registers" %s | cut -d \'=\' -f2'%(path),shell=True,).rstrip()
        self.PCRegsGold            = subprocess.check_output('grep "PC Register" %s | cut -d \'=\' -f2'%(path),shell=True,).rstrip()

    """File to save the current context"""
    def writeInformation(self,path):
        #Get in a line the number of instructions per core
        self.allCoreInsts=str(int(self.perCoreInsts[0]))
        for i in range(1,self.options.num_cpus):
            self.allCoreInsts+=","+str(int(self.perCoreInsts[i]))

        path=path+"/"+self.options.goldinformation
        print path
        fileptr = open(path,'w')

        fileptr.write("Application # of instructions={0}\n".format(self.executedInstructions))
        fileptr.write("Kernel Boot # of instructions={0}\n".format(self.linuxBootInstructions))
        fileptr.write("Number of Ticks={0}\n".format(self.executedTicks))

        fileptr.write("Integer Registers={0}\n".format(self.integerRegisters))
        fileptr.write("Float Registers  ={0}\n".format(self.floatRegisters))
        fileptr.write("PC Register={0}\n".format(self.PCRegister))
        fileptr.write("Executed Instructions Per core={0}\n".format(self.allCoreInsts))
        fileptr.close()

    """Get number of executed instructions in all cores"""
    def totalSimulatedInst(self):
        totalInstructions=0
        for i in range(0,self.options.num_cpus):
            totalInstructions+=int(self.system.cpu[i]._ccObject.totalInsts())
        return totalInstructions

    """Simulation manager"""
    def simulate(self,m5):
        self.m5 = m5
        #Define what checkpoint will be restored if is the gold execution to acquire checkpoints or the fault campaign
        checkpointDir=self.checkpointDirectory()

        print "checkpointDir ",checkpointDir
        #Restore the checkpoint
        m5.instantiate(checkpointDir)

        #Setup the fault
        if self.options.faultcampaignrun:
            self.setFault(faultNumber = 0)

        #setup profiling
        if self.options.checkpointinterval!=0 and self.options.goldrun:
            self.setProfilingGold()

        #While until the application ends
        exitCondition  = True
        self.exitCause = None

        #Tick that was loaded
        self.loadedTick = m5.curTick();

        if self.options.faultcampaignrun:
            while exitCondition:
                exitEvent = m5.simulate(int(self.TicksGold)*2)
                self.exitCause = exitEvent.getCause()

                print '**************************************************************'
                print 'Stop @ tick %i @ instruction %i because %s' % (m5.curTick(),self.totalSimulatedInst(), self.exitCause)

                if self.exitCause == simulationStatesE.FIM_BEFORE_INJECTION.name:
                    exitCondition = self.injectFault()

                if self.exitCause == "GenericPageTableFault" or self.exitCause == "GenericAlignmentFault" or self.exitCause =="Segmentation fault":
                    exitCondition = False

                #Hang triggered by the number of instructions
                if self.exitCause==simulationStatesE.FIM_WAIT_HANG.name:
                    exitCondition = False

                #Hang triggered by the number of ticks
                if self.exitCause == "simulate() limit reached":
                    exitCondition = False

                #Segmentation fault in to the host linux in full system mode
                if self.exitCause == "m5_fail instruction encountered" and self.options.fullsystem:
                   exitCondition = False

                #Simulation reache the exit()
                if self.exitCause == "m5_exit instruction encountered" or self.exitCause == "target called exit()":
                    exitCondition = False

                print '**************************************************************'
        else:
            while exitCondition:
                exitEvent = m5.simulate()

                self.exitCause = exitEvent.getCause()

                print '**************************************************************'
                print 'Stop @ tick %i @ instruction %i because %s Code %d' % (m5.curTick(),self.totalSimulatedInst(), self.exitCause, exitEvent.getCode())
                print 'Stop %d' % (self.system.cpu[0]._ccObject.totalInsts())

                #Profile
                if self.exitCause==simulationStatesE.FIM_GOLD_PROFILING.name or self.exitCause=="checkpoint" or self.exitCause == "simulate() limit reached" :
                    self.profilingGold(self.exitCause)

                #Segmentation fault
                if self.exitCause == "m5_fail instruction encountered" and self.options.fullsystem:
                   print "The gold application was unsuccessful - seg fault"
                   sys.exit(1)

                #Simulation ended well
                if self.exitCause == "target called exit()" \
                or self.exitCause == "switchcpu" \
                or self.exitCause == "m5_exit instruction encountered":
                    self.exitCause = "m5_exit instruction encountered"
                    exitCondition = False

                if self.exitCause == "all threads reached the max instruction count":
                    perCoreInsts=str(self.system.cpu[0]._ccObject.totalInsts())
                    for i in range(1,self.options.num_cpus):
                        perCoreInsts+=","+str(self.system.cpu[i]._ccObject.totalInsts())

                    print perCoreInsts

                print '**************************************************************'

        print 'Exiting @ tick %i because %s' % (m5.curTick(), self.exitCause)

    """Create the report after analyse the fault behavior"""
    def report(self):
        #Prevent the checkpoint drain in some case due Gem5 incompatibility of checkpoint into detailed mode in syscall emulation mode
        #~ if not self.options.fullsystem and self.options.cpu_type=="detailed":
        #~ if self.options.cpu_type=="detailed":
            #~ checkpointDrain = True
        #~ else:
        checkpointDrain = False
        #~ m5.drain()

        """Collect some informations about the execution using the C++ object"""
        #Temporary file
        tempfile=subprocess.check_output('mktemp')

        #Call the C++ FIM object that return internal context
        self.system.faultSystem.saveRegisterContext(tempfile)

        #Get the values from the temporary file
        self.integerRegisters = str(linecache.getline(tempfile,1).rstrip().split()).strip('[]').replace(',','').replace('\'','')
        self.floatRegisters   = str(linecache.getline(tempfile,2).rstrip().split()).strip('[]').replace(',','').replace('\'','')
        self.PCRegister       = str(linecache.getline(tempfile,3).rstrip().split()).strip('[]').replace(',',' ').replace('\'','').rstrip()

        #Number of instructions for all cores
        self.executedInstructions = self.totalSimulatedInst()

        #Recover the total of executed instructions for ALL CORES and add the checkpoint entry
        self.perCoreInsts = np.zeros(self.options.num_cpus)
        for i in range(self.options.num_cpus):
            self.perCoreInsts[i]=self.system.cpu[i]._ccObject.totalInsts() + self.correctCheckpoint[i]

        #Number of ticks
        self.executedTicks = int(m5.curTick()) - self.loadedTick

        #Calculate the number of saved instructions using the checkpoint
        savedInstructions=0
        for i in range(0,self.options.num_cpus):
            savedInstructions+=int(self.correctCheckpoint[i])

        """Gold execution during the extended checkpoint process"""
        if self.options.goldrun and self.options.platformid!=0:
            self.executedInstructions+=savedInstructions
            self.writeInformation(self.checkpointFile)
            m5.checkpoint(self.checkpointFile.replace("FIM.Profile","END.Profile"),checkpointDrain)

        """Create the gold version in the first gold execution and first checkpoint creation"""
        if self.options.goldrun and self.options.platformid==0:
            if self.options.checkpointinterval==0:
                print "Gold end"
                m5.checkpoint(joinpath(self.cptdir,"gold_execution"),checkpointDrain)
                self.writeInformation(self.applicationCurrentFolder)
            else:
                path=joinpath(self.cptdir,"gold_execution_checkpoint")
                m5.checkpoint(path,checkpointDrain)
                self.writeInformation(path)

        """Check for errors"""
        if self.options.faultcampaignrun:
            print "FIM: Number of instructions %s Gold %s " % ((self.executedInstructions+savedInstructions),self.instructionsGold)

            #Default until proven wrong
            self.faultAnalysis = errorAnalysis.Masked_Fault
            Self.memInconsistency = "Null"

            #See if a fault arised and the type
            if self.exitCause == "target called exit()" or self.exitCause == "m5_exit instruction encountered":

                #Produce the checkpoint for this execution
                m5.checkpoint(joinpath(self.cptdir, "FIM.results.%d.%d" % (self.options.platformid,int(self.faultIndex))),checkpointDrain)

                memInconsistency=self.testMemory()

                if  (int(self.executedInstructions+savedInstructions) != int(self.instructionsGold) or (self.PCRegister != self.PCRegsGold)):
                    print bcolors.WARNING+"FIM: Error number of instructions distinct %s Gold %s "%((self.executedInstructions+savedInstructions),self.instructionsGold) +bcolors.ENDC
                    print bcolors.WARNING+"FIM: Error PC register distinct %s Gold %s "%((self.PCRegister),self.PCRegsGold) +bcolors.ENDC

                    #~ for i in range(0,self.options.num_cpus):
                        #~ print "{0} {1}".format(int(self.system.cpu[i]._ccObject.totalInsts()),int(self.perCoreGoldCount[i]))

                    if memInconsistency:
                        self.faultAnalysis = errorAnalysis.Control_Flow_Data_ERROR
                    else:
                        self.faultAnalysis = errorAnalysis.Control_Flow_Data_OK
                elif self.FloatRegsGold != self.floatRegisters:
                    print "FIM: Error registers context %s %s" % (self.FloatRegsGold,self.floatRegisters)

                    if memInconsistency:
                        self.faultAnalysis = errorAnalysis.REG_STATE_Data_ERROR
                    else:
                        self.faultAnalysis = errorAnalysis.REG_STATE_Data_OK
                elif self.IntegerRegsGold != self.integerRegisters:
                    print "FIM: Error registers context \n Gold %s \n Result %s" % (self.IntegerRegsGold,self.integerRegisters)

                    if memInconsistency:
                        self.faultAnalysis = errorAnalysis.REG_STATE_Data_ERROR
                    else:
                        self.faultAnalysis = errorAnalysis.REG_STATE_Data_OK
                elif memInconsistency:
                    self.faultAnalysis = errorAnalysis.silent_data_corruption
                else:
                    self.faultAnalysis = errorAnalysis.Masked_Fault
            
                #Retest the memory to search 
                if self.faultType==faultTypesE.memory.name:
                    # Undo the fault
                    self.system.faultSystem.faultInjectionHandler()
                    
                    # Produce the checkpoint for this execution
                    m5.checkpoint(joinpath(self.cptdir, "FIM.results.%d.%d" % (self.options.platformid,int(self.faultIndex))),checkpointDrain)
                    
                    # Retest the memory
                    memretest=self.testMemory()
                    
                    # If the memory was inconsist before 
                    if not memretest and memInconsistency:
                        self.faultAnalysis = errorAnalysis.silent_data_corruption2
            
            #Possible infinite loop detected
            elif self.exitCause == simulationStatesE.FIM_WAIT_HANG.name:
                self.faultAnalysis=errorAnalysis.Hang
            #Page fault error
            elif self.exitCause == "GenericPageTableFault":
                self.faultAnalysis=errorAnalysis.RD_PRIV
            #Alignment error
            elif self.exitCause == "GenericAlignmentFault":
                self.faultAnalysis=errorAnalysis.RD_ALIGN
            #Segmentation fault
            elif self.exitCause == "m5_fail instruction encountered":
                self.faultAnalysis=errorAnalysis.SEG_FAULT
            #Simulation stop fetching new instructions and just ticks
            elif self.exitCause == "simulate() limit reached":
                self.faultAnalysis=errorAnalysis.FE_ABORT
            #Not yet rated
            else:
                 print "FIM: Abnormal exit without classification"
                 self.faultAnalysis=errorAnalysis.Unknown
        
            #Remove the simulation files
            subprocess.check_output("rm -rf {0}".format(self.applicationOutputFolder), shell=True)

            #Report file
            fileptr = open("{0}/Reports/reportfile-{1}".format(self.applicationCurrentFolder,self.options.platformid),'w')

            fileptr.write("%8s %7s %10s %2s %18s %7s %28s %6s %25s %22s %18s %12s\n"%(
                    "Index","Type","Target","i","Insertion Time","Mask","Fault Injection Result",
                    "Code", "Execution Time (Ticks)", "Executed Instuctions", "Mem Inconsistency",
                    "Checkpoint"))

            fileptr.write("%7s %10s %6s %4s %15s %12s %25s %7s %20s %22s %14s %18s\n" % (
                            self.faultIndex,
                            self.faultType,
                            self.faultRegister,
                            self.faultRegisterIndex,
                            self.faultTime,
                            self.faultMask,
                            self.faultAnalysis.name,
                            str(self.faultAnalysis.value),
                            self.executedTicks,
                            self.executedInstructions,
                            Self.memInconsistency,
                            str(savedInstructions)
                            ))

            fileptr.close()
