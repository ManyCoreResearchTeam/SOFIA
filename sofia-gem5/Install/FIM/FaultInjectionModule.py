""" 
Fault Injection Module to Gem5
Version 1.0
02/02/2016
    
Author: Felipe Rocha da Rosa - frdarosa.com
"""
import sys
from os import getcwd
from os.path import join as joinpath

import CpuConfig
import MemConfig

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.util import *

#My imports
from array import *
from enum import Enum
import subprocess
import linecache
import ctypes
import numpy as np
from operator import itemgetter

addToPath('../../config/common')
addToPath('../../config/ruby')
addToPath('../../../Support/Tools')

from FaultClassification import *

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
    platformIdentifier = 0              # Platform unique identifier
    faultIndex = 0                      # Account what the current fault to be injected
    faultList = []                      # List of faults to be injected

    #Possible register list 
    possibleRegisters = ["r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12","r13","r14","pc"]

    #Number of registers
    numberOfRegisters = len(possibleRegisters)-1
    
    """Class Creator""" 
    def __init__(self):
        self.LinuxBootInstructions=0
        self.HangOverRun=1.5
        #Get current folder
        self.ApplicationCurrentFolder = subprocess.check_output('pwd',shell=True,).rstrip()
    
    """Initialize the module"""
    def FIMcreation(self,options,system,cptdir):
        self.options=options
        self.system=system
        self.platformIdentifier = self.options.FIM_platform_ID
        self.cptdir = cptdir
        self.InstructionsGoldMax = np.zeros(options.num_cpus)
        self.correctCheckpoint = np.zeros(options.num_cpus)
        
        
        #verify arguments compatibility
        if self.options.FIM_fault_campaign_run and self.options.FIM_gold_run:
            fatal("Can't specify both --FIM_fault_campaign_run --FIM_gold_run")
            sys.exit(1)
                
        if options.FIM_gold_run:
            self.State = simulationStates.FIM_GOLD_EXECUTION
            self.faultCoreToInject = 0
        
        #Acquire the list of fault to this platform
        if options.FIM_fault_campaign_run:
            self.State = simulationStates.FIM_BEFORE_INJECTION
            
            #Retrive the information about the gold execution
            self.retriveInformation(self.ApplicationCurrentFolder)
            
            # If this maxinsts was triggered during the fault injection means that the applications reached more than 50% the
            # normal value extract from the fault free execution. To classification purpose, will be considered that the
            # application is in a infinite loop. Each core will have a distinct value?
            for i in range(options.num_cpus):
                self.InstructionsGoldMax[i]=int(int(self.PerCoreGoldCount[i])*self.HangOverRun)
            
            #determine the correct line of the file
            linePosition = ((self.platformIdentifier-1)*self.options.FIM_number_of_faults)+1;

            #Collect the faults (one per default)
            for i in range(1,self.options.FIM_number_of_faults+1):
                self.faultList.append(linecache.getline(self.options.FIM_fault_list, linePosition+i).split())
            
            #Core to inject
            self.faultCoreToInject=int(self.faultList[0][4].split(":")[-1])
            #~ self.faultCoreToInject=0 #REVIEW
            
            #Remove the core value and the :
            self.faultList[0][4]=self.faultList[0][4].split(":")[0]
        
        #Create the C++ object to interact during the simulation
        self.system.faultSystem = FaultInjectionSystem(system=self.system.cpu[self.faultCoreToInject], CPU_Type=(1 if self.options.cpu_type=="detailed" else 0))
                 
    """Add the FIModule options to the parser"""
    def optionsParser(self,parser):
        parser.add_option("--FIM_gold_run",             action="store_true", help="Gold Execution")
        parser.add_option("--FIM_full_system",          action="store_true", help="Full System")
        parser.add_option("--FIM_kernel_benchmark",     action="store_true", help="Create a checkpoit at the end of the linux boot")
        parser.add_option("--FIM_checkpoint_switch",    action="store_true", help="Create multiple checkpoints using the detailed mode")
        parser.add_option("--FIM_fault_campaign_run",   action="store_true", help="Fault Campaign Execution")
        
        parser.add_option("--FIM_number_of_faults",     action="store",     type="int",     dest="FIM_number_of_faults",    help="Number of faults to be injected", default=1)
        parser.add_option("--FIM_checkpoint_profile",   action="store",     type="int",     dest="FIM_checkpoint_profile",  help="Create periodical checkpoints during the gold execution", default=0)
        parser.add_option("--FIM_platform_ID",          action="store",     type="int",     dest="FIM_platform_ID",         help="Unique platform identifier", default=0)
        parser.add_option("--FIM_fault_list",           action="store",     type="string",  dest="FIM_fault_list",          help="File containign the fault list", default="./FAULT_list")
        parser.add_option("--FIM_kernel_checkpoint",    action="store",     type="string",  dest="FIM_kernel_checkpoint",   help="Checkpoint for full system")
        parser.add_option("--FIM_project_folder",       action="store",     type="string",  dest="FIM_project_folder",      help="Project folder")

    """Set the next fault """
    def setFault(self,faultNumber):
        self.faultIndex             = self.faultList[faultNumber][0]
        self.faultType              = self.faultList[faultNumber][1]
        self.faultRegister          = self.faultList[faultNumber][2]
        self.faultRegisterIndex     = self.faultList[faultNumber][3]
        self.faultTime              = self.faultList[faultNumber][4]
        self.faultMask              = self.faultList[faultNumber][5]
        
        #Required after m5.instantiate(checkpoint_dir)! --Fault time should be adjusted to use checkpoint and -1
        self.system.cpu[self.faultCoreToInject].scheduleInstStop(0,int(int(self.faultTime)-int(self.correctCheckpoint[self.faultCoreToInject])-1), simulationStates.FIM_BEFORE_INJECTION.name)
        
        if self.faultRegister=="pc":
            self.targetPC=True
        else:
            self.targetPC=False
    
    """Insert the fault into the processor"""
    def injectFault(self):
        #Verify the actual state of the fault simulation
        if self.State == simulationStates.FIM_BEFORE_INJECTION:
            
            #Inject the fault using the C++ object int faultMask, int faultType, int faultRegister, bool targetPC
            self.system.faultSystem.faultInjectionHandler(ctypes.c_int32(int(self.faultMask,16)).value, 0, int(self.faultRegisterIndex), self.targetPC)
            
            #Adjust the scheduler to trigger the next state
            for i in range(self.options.num_cpus):
                if self.InstructionsGoldMax[i]!=0:
                    stopPoint = self.InstructionsGoldMax[i] - self.system.cpu[i]._ccObject.totalInsts()
                    self.system.cpu[i].scheduleInstStop(0,int(stopPoint),simulationStates.FIM_WAIT_HANG.name)
            
            #Next state
            self.State = simulationStates.FIM_WAIT_HANG
            return True 
    
    """Create checkpoint in a interval"""
    def setProfilingGold(self):
        self.gap = self.options.FIM_checkpoint_profile - (self.options.FIM_checkpoint_profile%1000000)
        print self.gap
        
        #Schedule the next checkpoint
        self.system.cpu[0].scheduleInstStop(0,int(self.gap)-1,simulationStates.FIM_GOLD_PROFILING.name)
    
    """Creates the checkpoint during the simulation"""
    def profilingGold(self,exit_cause):
        #Profile an application
        if exit_cause==simulationStates.FIM_GOLD_PROFILING.name or self.exit_cause == "simulate() limit reached":
            
            print "FIM: Checkpoit at %d" %self.totalSimulatedInst()
            
            if self.options.FIM_checkpoint_switch:
                self.m5.switchCpus(self.system, self.switch_cpu_list)
            
            #Schedule the next checkpoint            
            self.system.cpu[0].scheduleInstStop(0,int(self.gap),simulationStates.FIM_GOLD_PROFILING.name)
            
            #Checkpoint name
            CheckpointName="FIM.Profile."+str(self.system.cpu[0]._ccObject.totalInsts())
            for i in range(1,self.options.num_cpus):
                CheckpointName+="."+str(self.system.cpu[i]._ccObject.totalInsts())
            
            print CheckpointName
            #Create checkpoint
            m5.checkpoint(joinpath(self.cptdir,CheckpointName),False)
            
            if self.options.FIM_checkpoint_switch:
                tmp_cpu_list = []
                for old_cpu, new_cpu in self.switch_cpu_list:
                    tmp_cpu_list.append((new_cpu, old_cpu))
                self.switch_cpu_list = tmp_cpu_list
                
                self.m5.switchCpus(self.system, self.switch_cpu_list)
                
                tmp_cpu_list = []
                for old_cpu, new_cpu in self.switch_cpu_list:
                    tmp_cpu_list.append((new_cpu, old_cpu))
                self.switch_cpu_list = tmp_cpu_list
                
        #checkpoint message coming from the linux --The first will be just after the linux boot
        if exit_cause=="checkpoint":
            print "FIM: Application beggan at %d" %self.totalSimulatedInst()
            
            #Create checkpoint
            m5.checkpoint(joinpath(self.cptdir,"FIM.After_Kernel_Boot.%d"%(self.totalSimulatedInst())))
            
            #Number of instructions to boot
            self.LinuxBootInstructions = int(self.totalSimulatedInst())
            
            #Exit here - checkpoint ok
            sys.exit()
    
    """ Test if the memory was correct after compare with the gold run"""
    def testMemory(self):
        MemInconsistency = False
        
        #Select the gold run memory to compare
        #Check if a checkpoint was reloade
        if self.CheckpointReloaded==False:
            path="{0}/m5out/gold_execution".format(self.ApplicationCurrentFolder)
        else:
            #Each checkpoint in the detailed mode has a memory
            if self.options.cpu_type=="detailed":
                self.InstructionsGold=int(self.InstructionsGold)
                path=self.CheckpointFile.replace("FIM","END")
            else:
                path="{0}/m5out/gold_execution".format(self.ApplicationCurrentFolder)
                #~ path="{0}/m5out/gold_execution_checkpoint".format(self.ApplicationCurrentFolder)
        
        #Get the number of memories
        NumberOfMemories =int(subprocess.check_output('find {0} -name "system.physmem.store*" | wc -l'.format(path),shell=True,)) 
        
        #Verify the consistency of the memory and caches
        for i in range(NumberOfMemories):
            CompareMemCmd="cmp {0}/{1}/FIM.results.{2}.{3}/system.physmem.store{4}.pmem {5}/system.physmem.store{4}.pmem"\
            .format(self.ApplicationCurrentFolder,self.cptdir,self.options.FIM_platform_ID,int(self.faultIndex),i,path)
            print CompareMemCmd
            
            try:
                MemInconsistency = subprocess.check_output(CompareMemCmd,shell=True,)
            except subprocess.CalledProcessError as e:
                print bcolors.WARNING+"ERROR !!!!!!!!"+bcolors.ENDC
                MemInconsistency = True
        
        if MemInconsistency:
            print bcolors.WARNING+"ERROR !!!!!!!!"+bcolors.ENDC
            
        #In expecial cases...
        if self.options.cpu_type=="atomic" and self.CheckpointReloaded and MemInconsistency and self.options.num_cpus>2:
            MemInconsistency = False
            path="{0}/m5out/gold_execution_checkpoint".format(self.ApplicationCurrentFolder)
            for i in range(NumberOfMemories):
                CompareMemCmd="cmp {0}/{1}/FIM.results.{2}.{3}/system.physmem.store{4}.pmem {5}/system.physmem.store{4}.pmem"\
                .format(self.ApplicationCurrentFolder,self.cptdir,self.options.FIM_platform_ID,int(self.faultIndex),i,path)
                print CompareMemCmd
                
                try:
                    MemInconsistency = subprocess.check_output(CompareMemCmd,shell=True,)
                except subprocess.CalledProcessError as e:
                    MemInconsistency = True
        
        if MemInconsistency:
            Self.MemInconsistency = "YES"
            print bcolors.WARNING+"FIM: Memory inconsistency"+bcolors.ENDC
        else:
            Self.MemInconsistency = "NO"
        
        return MemInconsistency

    """Finds the best checkpoint for this fault"""
    def checkpointDirectory(self):
        #Departure point without checkpoint
        if self.options.FIM_full_system and not self.options.FIM_kernel_benchmark and len(self.options.FIM_kernel_checkpoint):
            #Load the checkpoint after the linux boot
            self.CheckpointFile = subprocess.check_output('find %s -name "FIM.After_Kernel_Boot*"'%(self.options.FIM_kernel_checkpoint),shell=True,).rstrip()
        else:
            #No checkpoint necessary
            self.CheckpointFile = None

        #A checkpoint in the midle of the application was reloaded? dafault false
        self.CheckpointReloaded = False
        
        """During the Fault Campaing others checkpoits could be reloaded"""
        if self.options.FIM_fault_campaign_run and self.options.FIM_checkpoint_profile!=0:
            #get all possible checkpoints
            GetCheckpointFiles = subprocess.check_output('find . -name "FIM.Profile.*"',shell=True,).split()
            #Get the numbers
            GetCheckpointFiles = [item.split(".") for item in GetCheckpointFiles]
            
            #Sort the list acording the faultCoreToInject
            GetCheckpointFiles = sorted(GetCheckpointFiles,key=lambda i: (int(i[self.faultCoreToInject+3])))
            
            #Checkpoints that could be restore in this case
            possibleCheckpoint=[int(item[self.faultCoreToInject+3]) for item in GetCheckpointFiles if int(item[self.faultCoreToInject+3]) < int(self.faultList[0][4])]
            
            """A checkpoint in the middle of the application was chosen"""
            if len(possibleCheckpoint)!=0:
                #Select the best checkpoint if is the case
                Checkpoint=possibleCheckpoint.index(possibleCheckpoint[-1])
                
                #OBS:the +3 is due the file name after the split ./FIM.Profile.XXXX
                del GetCheckpointFiles[Checkpoint][0]
                del GetCheckpointFiles[Checkpoint][0]
                del GetCheckpointFiles[Checkpoint][0]
                
                self.CheckpointReloaded = True
                #Checkpoint name 
                CheckpointName="FIM.Profile"
                for i in range(self.options.num_cpus):
                    CheckpointName+="."+GetCheckpointFiles[Checkpoint][i]
                    self.correctCheckpoint[i]=int(GetCheckpointFiles[Checkpoint][i])
                
                self.CheckpointFile=joinpath(self.cptdir,"%s/m5out/%s"%(self.ApplicationCurrentFolder,CheckpointName))
                
                """Retrive the information according with the correct checkpoint"""
                if self.options.cpu_type=="detailed" and not self.options.FIM_checkpoint_switch:
                    self.retriveInformation(self.CheckpointFile)
                #~ elif self.options.cpu_type=="atomic":
                    #~ self.retriveInformation("%s/m5out/gold_execution_checkpoint" % (self.ApplicationCurrentFolder))
                
                # If this maxinsts was triggered during the fault injection means that the applications reached more than 50% the
                # normal value extract from the fault free execution. To classification purpose, will be considered that the
                # application is in a infinite loop (hang)
                for i in range(0,self.options.num_cpus):
                    self.PerCoreGoldCount[i]=int(self.PerCoreGoldCount[i]) - self.correctCheckpoint[i]
                    self.InstructionsGoldMax[i]=int(self.PerCoreGoldCount[i]*self.HangOverRun)
                    
        """During the extend checkpoint creation to detailed cpu"""
        if self.options.FIM_gold_run and self.options.FIM_platform_ID!= 0:
            #get possible checkpoints
            GetCheckpointFiles = subprocess.check_output('find . -name "FIM.Profile.*"',shell=True,).split()
            self.CheckpointFile=GetCheckpointFiles[int(self.options.FIM_platform_ID)-1].split("./")[-1]
            
            #Get the values of instruction where this platform will be loaded
            checkpointValue=self.CheckpointFile.split(".")
            del checkpointValue[0]
            del checkpointValue[0]
            
            for i in range(self.options.num_cpus):
                self.correctCheckpoint[i]=int(checkpointValue[i])
            
            self.CheckpointFile=joinpath(self.cptdir, "%s/%s" % (self.ApplicationCurrentFolder,self.CheckpointFile))
        return self.CheckpointFile
    
    """Retrive the gold information""" 
    def retriveInformation(self,path):
        path=path+"/Information"
        self.InstructionsGold      = subprocess.check_output('grep "Application # of instructions" %s | cut -d \'=\' -f2'%(path),shell=True,).rstrip()
        self.LinuxBootInstructions = subprocess.check_output('grep "Kernel Boot # of instructions" %s | cut -d \'=\' -f2'%(path),shell=True,).rstrip()
        self.TicksGold             = subprocess.check_output('grep "Number of Ticks" %s | cut -d \'=\' -f2'%(path),shell=True,).rstrip()
        self.PerCoreGoldCount      = subprocess.check_output('grep "Executed Instructions Per core=" %s | cut -d \'=\' -f2'%(path),shell=True,).rstrip().split(",")
        
        self.FloatRegsGold         = subprocess.check_output('grep "Float Registers" %s | cut -d \'=\' -f2'%(path),shell=True,).rstrip()
        self.IntegerRegsGold       = subprocess.check_output('grep "Integer Registers" %s | cut -d \'=\' -f2'%(path),shell=True,).rstrip()
        self.PCRegsGold            = subprocess.check_output('grep "PC Register" %s | cut -d \'=\' -f2'%(path),shell=True,).rstrip()
    
    """File to save the current context""" 
    def writeInformation(self,path):
        #Get in a line the number of instructions per core
        self.allCoreInsts=str(int(self.PerCoreInsts[0]))
        for i in range(1,self.options.num_cpus):
            self.allCoreInsts+=","+str(int(self.PerCoreInsts[i]))
            
        fileptr = open("%s/Information"%path,'w')
        fileptr.write("Application # of instructions={0}\n".format(self.ExecutedInstructions))
        fileptr.write("Kernel Boot # of instructions={0}\n".format(self.LinuxBootInstructions))
        fileptr.write("Number of Ticks={0}\n".format(self.ExecutedTicks))
        
        fileptr.write("Integer Registers={0}\n".format(self.integerRegisters))
        fileptr.write("Float Registers={0}\n".format(self.floatRegisters))
        fileptr.write("PC Register={0}\n".format(self.PCRegister))
        fileptr.write("Executed Instructions Per core={0}\n".format(self.allCoreInsts))
        fileptr.close()
    
    """File to save the current context"""  
    def totalSimulatedInst(self):
        TotalInsts=0
        for i in range(0,self.options.num_cpus):
            TotalInsts+=int(self.system.cpu[i]._ccObject.totalInsts())
        return TotalInsts
           
    """Simulation mananger""" 
    def simulate(self,m5):
        self.m5 = m5
        
        #Switch between Detailed and atomic cpu type during the checkpoint creation
        if self.options.FIM_checkpoint_switch:
            np = self.options.num_cpus
            self.switch_cpus = [AtomicSimpleCPU(switched_out=True, cpu_id=(i)) for i in xrange(np)]
            for i in xrange(np):
                self.switch_cpus[i].system =  self.system
                self.switch_cpus[i].workload = self.system.cpu[i].workload
                self.switch_cpus[i].clk_domain = self.system.cpu[i].clk_domain

                self.switch_cpus[i].max_insts_any_thread = 0

            self.system.switch_cpus = self.switch_cpus
            self.switch_cpu_list = [(self.system.cpu[i], self.switch_cpus[i]) for i in xrange(np)]
        
        #Define what checkpoit will be restored if is the gold execution to acquire checkpoints or the fault campaign
        checkpoint_dir=self.checkpointDirectory()
        
        print checkpoint_dir
        #Restore the checkpoint
        m5.instantiate(checkpoint_dir)

        #Setup the fault
        if self.options.FIM_fault_campaign_run:
            self.setFault(faultNumber = 0)
        
        #setup profiling
        if self.options.FIM_checkpoint_profile!=0 and self.options.FIM_gold_run:
            self.setProfilingGold()
        
        #While until the application ends
        exit_condition = True
        self.exit_cause = None
        
        #Tick that was loaded
        self.LoadedTick = m5.curTick();
        
        if self.options.FIM_fault_campaign_run:
            while exit_condition:
                exit_event = m5.simulate(int(self.TicksGold)*2)
                self.exit_cause = exit_event.getCause()
                
                print '**************************************************************'
                print 'Stop @ tick %i @ instruction %i because %s' % (m5.curTick(),self.totalSimulatedInst(), self.exit_cause)
                
                if self.exit_cause == simulationStates.FIM_BEFORE_INJECTION.name:
                    exit_condition = self.injectFault()
                
                if self.exit_cause == "GenericPageTableFault" or self.exit_cause == "GenericAlignmentFault" or self.exit_cause =="Segmentation fault": 
                    exit_condition = False
                
                #Hang trigerred by the number of instructions 
                if self.exit_cause==simulationStates.FIM_WAIT_HANG.name:
                    exit_condition = False
                
                #Hang trigerred by the number of ticks
                if self.exit_cause == "simulate() limit reached":
                    exit_condition = False
                
                #Segmentetion fault in to the host linux in full system mode
                if self.exit_cause == "m5_fail instruction encountered" and self.options.FIM_full_system:
                   exit_condition = False
                
                #Simulation reache the exit()
                if self.exit_cause == "m5_exit instruction encountered" or self.exit_cause == "target called exit()":
                    exit_condition = False
                    
                print '**************************************************************'
        else:
            while exit_condition:
                #~ if self.options.FIM_checkpoint_profile==0 :
                exit_event = m5.simulate()
                #~ else:
                    #~ exit_event = m5.simulate(int(self.gap))
                #~ exit_event = m5.simulate( int(self.gap) if (self.options.FIM_checkpoint_profile!=0 and self.options.num_cpus > 1) else m5.MaxTick)
                
                #~ exit_event = m5.simulate( int(self.gap) if self.options.FIM_checkpoint_profile!=0 and self.options.num_cpus > 1 else m5.MaxTick)
                self.exit_cause = exit_event.getCause()
                
                print '**************************************************************'
                print 'Stop @ tick %i @ instruction %i because %s Code %d' % (m5.curTick(),self.totalSimulatedInst(), self.exit_cause, exit_event.getCode())
                print 'Stop %d' % (self.system.cpu[0]._ccObject.totalInsts())
                
                #Profile
                if self.exit_cause==simulationStates.FIM_GOLD_PROFILING.name or self.exit_cause=="checkpoint" or self.exit_cause == "simulate() limit reached" :
                    self.profilingGold(self.exit_cause)
                
                #Segmentetion fault
                if self.exit_cause == "m5_fail instruction encountered" and self.options.FIM_full_system:
                   print "The gold application was unssucceful - seg fault"
                   sys.exit(1)
                
                #Simulation ended well
                if self.exit_cause == "target called exit()" \
                or self.exit_cause == "switchcpu" \
                or self.exit_cause == "m5_exit instruction encountered":
                    self.exit_cause = "m5_exit instruction encountered"
                    exit_condition = False
                
                if self.exit_cause == "all threads reached the max instruction count":
                    PerCoreInsts=str(self.system.cpu[0]._ccObject.totalInsts())
                    for i in range(1,self.options.num_cpus):
                        PerCoreInsts+=","+str(self.system.cpu[i]._ccObject.totalInsts())
                    
                    print PerCoreInsts
                
                print '**************************************************************'
        
        print 'Exiting @ tick %i because %s' % (m5.curTick(), self.exit_cause)

    """Create the report after analyse the fault behavior"""
    def report(self):
        
        #Prevent the checkpoint drain in some case due Gem5 incompatibility of checkpoint into detailed mode in syscall emulation mode
        #~ if not self.options.FIM_full_system and self.options.cpu_type=="detailed":
        if self.options.cpu_type=="detailed":
            CheckpointDrain = False
        else:
            CheckpointDrain = True
        
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
        self.ExecutedInstructions = self.totalSimulatedInst()
        
        #Recover the total of executed instructions for ALL CORES and add the checkpoint entry
        self.PerCoreInsts = np.zeros(self.options.num_cpus)
        for i in range(self.options.num_cpus):
            self.PerCoreInsts[i]=self.system.cpu[i]._ccObject.totalInsts() + self.correctCheckpoint[i]
        
        #Number of ticks
        self.ExecutedTicks = int(m5.curTick()) - self.LoadedTick

        #Calculate the number of saved instructions using the checkpoint
        savedInstructions=0
        for i in range(0,self.options.num_cpus):
            savedInstructions+=int(self.correctCheckpoint[i])

        """Gold execution during the detailed checkpoint process"""
        if self.options.FIM_gold_run and self.options.FIM_platform_ID!=0:
            self.ExecutedInstructions+=savedInstructions
            self.writeInformation(self.CheckpointFile)
            m5.checkpoint(self.CheckpointFile.replace("FIM","END"),CheckpointDrain)
            

        """Create the gold version in the first gold execution and first checkpoint creation"""
        #~ if self.options.FIM_gold_run and self.options.FIM_checkpoint_profile==0 and self.options.FIM_platform_ID==0:
        if self.options.FIM_gold_run and self.options.FIM_platform_ID==0:
            if self.options.FIM_checkpoint_profile==0:
                m5.checkpoint(joinpath(self.cptdir,"gold_execution"),CheckpointDrain)
                self.writeInformation(self.ApplicationCurrentFolder)
            else:
                path=joinpath(self.cptdir,"gold_execution_checkpoint")
                m5.checkpoint(path,CheckpointDrain)
                self.writeInformation(path)
            
        """Check for errors"""
        if self.options.FIM_fault_campaign_run:
            print "FIM: Number of instructions %s Gold %s " % ((self.ExecutedInstructions+savedInstructions),self.InstructionsGold)
            
            #Default until proven wrong
            self.faultAnalysis = errorAnalysis.Masked_Fault
            Self.MemInconsistency = "Null"
            
            #See if a fault arised and the type
            if self.exit_cause == "target called exit()" or self.exit_cause == "m5_exit instruction encountered":
            
                #Produce the checkpoint for this execution
                m5.checkpoint(joinpath(self.cptdir, "FIM.results.%d.%d" % (self.options.FIM_platform_ID,int(self.faultIndex))),CheckpointDrain)
                
                MemInconsistency=self.testMemory()
                
                if  ((self.ExecutedInstructions+savedInstructions) != int(self.InstructionsGold) or (self.PCRegister != self.PCRegsGold)):
                    print bcolors.WARNING+"FIM: Error number of instructions distinct %s Gold %s "%((self.ExecutedInstructions+savedInstructions),self.InstructionsGold) +bcolors.ENDC
                    print bcolors.WARNING+"FIM: Error number of instructions distinct %s Gold %s "%((self.PCRegister),self.PCRegsGold) +bcolors.ENDC

                    if MemInconsistency:
                        self.faultAnalysis = errorAnalysis.Control_Flow_Data_ERROR
                    else:
                        self.faultAnalysis = errorAnalysis.Control_Flow_Data_OK
                elif self.FloatRegsGold != self.floatRegisters:
                    print "FIM: Error registers context %s %s" % (self.FloatRegsGold,self.floatRegisters)

                    if MemInconsistency:
                        self.faultAnalysis = errorAnalysis.REG_STATE_Data_ERROR
                    else:
                        self.faultAnalysis = errorAnalysis.REG_STATE_Data_OK
                elif self.IntegerRegsGold != self.integerRegisters:
                    print "FIM: Error registers context \n Gold %s \n Result %s" % (self.IntegerRegsGold,self.integerRegisters)

                    if MemInconsistency:
                        self.faultAnalysis = errorAnalysis.REG_STATE_Data_ERROR
                    else:
                        self.faultAnalysis = errorAnalysis.REG_STATE_Data_OK
                elif MemInconsistency:
                    self.faultAnalysis = errorAnalysis.silent_data_corruption
                else:
                    self.faultAnalysis = errorAnalysis.Masked_Fault
            #Possible infinite loop detected
            elif self.exit_cause == simulationStates.FIM_WAIT_HANG.name:
                self.faultAnalysis=errorAnalysis.Hang
            #Page fault error
            elif self.exit_cause == "GenericPageTableFault": 
                self.faultAnalysis=errorAnalysis.RD_PRIV
            #Alignment error
            elif self.exit_cause == "GenericAlignmentFault":
                self.faultAnalysis=errorAnalysis.RD_ALIGN
            #Segmentetion fault
            elif self.exit_cause == "m5_fail instruction encountered":
                self.faultAnalysis=errorAnalysis.SEG_FAULT
            #Simulation stop fetching new instructiions
            elif self.exit_cause == "simulate() limit reached":
                self.faultAnalysis=errorAnalysis.FE_ABORT
            #Not yet rated
            else:
                 print "FIM: Abnormal exit without classification"
                 self.faultAnalysis=errorAnalysis.Unknown
            
            #Report file
            fileptr = open("%s/Reports/FIM.report.%d.%d"%(self.ApplicationCurrentFolder,self.options.FIM_platform_ID,int(self.faultIndex)),'w')
            
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
                            self.ExecutedTicks,
                            self.ExecutedInstructions,
                            Self.MemInconsistency,
                            str(savedInstructions)
                            ))
            
            fileptr.close()
