#   Fault Injection Module Graphical Interface
#
#   Fault campaign class
#
#
#   Author: Felipe Rocha da Rosa


# Libraries
import subprocess
import sys
import os
import signal
import time
import shutil
import inspect
import datetime
import multiprocessing
import threading
import glob
import atexit

try:
    # for Python2
    from tkinter import *   ## notice capitalized T in Tkinter
except ImportError:
    # for Python3
    from tkinter import *   ## notice lowercase 't' in tkinter here

# Local classes
from options       import *
from commondefines import *
from faultlist     import *
from harvester     import *
from faultcampaign  import *
#
# Class: Main class
#
class faultCampaign():
    # Constructor
    def __init__(self,options=None,terminal=None):
        if options==None:
           fimMessages().errorMessageWithTermination(message="Missing Parameters!")
        self.options=options
        self.terminal=terminal
        #Send to /dev/null
        self.FNULL = open(os.devnull, 'w')
        atexit.register(self.exitCleaning)
        # Catch Crtl+C
        signal.signal(signal.SIGINT, self.signalHandler)

    # Captures the CRTL+C and kill the pool 
    def signalHandler(self,signal,frame):
        print('You pressed Ctrl+C!')
        self.obj.terminate()

    # Exit
    def exitCleaning(self):
        # Back to the root folder
        os.chdir(self.dirproject)
        
        # Close queue
        self.writeTerminal(None)

    # Write in the terminal queue
    def writeTerminal(self,message=""):
        self.terminal.put(message)

    # Scroll terminal intercepting the children threads
    def terminalScroll(self,thread):
        while True:
            output = thread.stdout.readline() # <-- Hangs here!
            if not output:
                break
            self.writeTerminal(output)

    #
    # Environment configuration
    #
    # Create the appropriate configurations
    def configuration(self):
        # Acquire the current folder and derive some default paths
        self.dirproject=subprocess.check_output("pwd",shell=True,).rstrip('\n')
        self.dirworkspace=self.dirproject+"/workspace"
        self.dirworkload =self.dirproject+"/workloads"
        self.dirsupport  =self.dirproject+"/support"

        # Create workspace dir
        if not os.path.exists(self.dirworkspace):
            os.makedirs(self.dirworkspace)

        # virtual platform specific configuration
        if(self.options.environment==environmentsE.ovparmv8.name \
                or self.options.environment==environmentsE.ovparmv7.name):
            #@Review - Assuming OVP from here
            self.setupOVPFIM()

    # Configure the OVPsim-FIM environment
    def setupOVPFIM(self):
            if(os.environ.get("IMPERAS_HOME")==None):
                fimMessages().errorMessageWithTermination(message="OVPsim environment not configurated")
            self.dirplatform    = self.dirproject+"/platformOP"
            self.dirmodule      = self.dirplatform+"/module"
            self.dirintercpet   = self.dirplatform+"/intercept"
            self.dirharness     = self.dirplatform+"/harness"
            self.dirfimapi      = self.dirsupport+"/fim-api"
            self.cmdharness     = "harness.{0}.exe".format(os.environ.get("IMPERAS_ARCH"))
            self.cputype        = "OVP"

            # Simulator basic arguments
            self.cmdarguments="--faulttype {0} --targetcpucores {1} --interceptlib {2}/model.so"\
                    .format(self.options.faulttype,self.options.numcores,self.dirintercpet)

            # Workload configuration
            if (self.options.workload==workloadsE.baremetal.name):

                fimMessages().errorMessageWithTermination(message="Baremetal not supported!")
            elif (self.options.workload==workloadsE.freertos.name):

                fimMessages().errorMessageWithTermination(message="FreeRTOS not supported!")
            elif ([ True for item in linuxworkloadsL if item== self.options.workload]):

                #ARMv7
                if self.options.environment == environmentsE.ovparmv7.name:
                    # MPI suport library
                    self.mpilib="{0}/mpich-3.2-armv7".format(self.dirsupport)
                    self.mpicaller="armv7-linux-gnueabi-mpirun"

                    # Linux suport
                    self.dirlinux           = self.dirsupport+"/linux-armv7"
                    self.dirlinuximages     = self.dirlinux+"/disks"
                    self.linuxsourceimage   = "fs.img"
                    self.linuximage         = "{0}.{1}.{2}.{3}".format(self.linuxsourceimage,self.options.workload,self.options.environment,self.options.numcores)
                    self.linuxkernel        = "zImage.3.13.0"
                    self.linuxvkernel       = "vmlinux.3.13.0"
                    self.linuxbootloader    = "smpboot.ARM_CORTEX_A9.elf"
                    self.linuxguestfolder   = "root"
                    self.linuxscript        = "run.sh"
                    self.linuxfimapi        = "OVPFIM-ARMv7.elf"

                    # Copy the correct tcl platform
                    shutil.copy(self.dirmodule+"/module.op.tcl.linux",self.dirmodule+"/module.op.tcl")

                    # Verify is the boot loader is compiled
                    if( not os.path.isfile(self.dirlinuximages+"/"+self.linuxbootloader)):
                        fimMessages().debugMessage(message="Recompiling the bootloader {0}:".format(self.linuxbootloader))
                        subprocess.call("make -C "+self.dirlinuximages,shell=True);

                    # Update the simulator arguments
                    self.cmdarguments+=" --mode linux --startaddress 0x60000000 --objfilenoentry "+self.linuxbootloader

                    # Other flags
                    cpuvariant="Cortex-A9MPx"+str(self.options.numcores)
                    mpuflag="cortex-a9"
                    self.cpuarchitecture="ARM_CORTEX_A9"

                    # Export environment variables to fill the makefiles
                    os.environ["ARM_TOOLCHAIN_LINUX"]="/usr"
                    os.environ["ARM_TOOLCHAIN_LINUX_PREFIX"]="arm-linux-gnueabi"

                #ARMv8
                if self.options.environment == environmentsE.ovparmv8.name:

                    # MPI suport library
                    self.mpilib="{0}/mpich-3.2-armv8".format(self.dirsupport)
                    self.mpicaller="armv8-linux-gnueabi-mpirun"

                    # Linux suport
                    self.dirlinux           = self.dirsupport+"/linux-armv8"
                    self.dirlinuximages     = self.dirlinux+"/disks"
                    self.linuxsourceimage   = "initrd.arm64.img"
                    self.linuximage         = "{0}.{1}.{2}.{3}".format(self.linuxsourceimage,self.options.workload,self.options.environment,self.options.numcores)
                    self.linuxkernel        = "Image"
                    self.linuxvkernel       = "vmlinux"
                    self.linuxguestfolder   = "root"
                    self.linuxscript        = "run.sh"
                    self.linuxfimapi        = "OVPFIM-ARMv8.elf"

                    # Copy the correct tcl platform
                    shutil.copy(self.dirmodule+"/module.op.tcl.linux64",self.dirmodule+"/module.op.tcl")
                    
                    # Other flags
                    if   self.options.architecture == architecturesE.cortexA53.name:
                        self.linuxdtb="foundation-v8.dtb"
                        cpuvariant="Cortex-A53MPx"+str(self.options.numcores)
                        mpuflag="cortex-a53"
                        self.cpuarchitecture="ARM_CORTEX_A53"
                    elif self.options.architecture == architecturesE.cortexA57.name:
                        self.linuxdtb="foundation-v8.dtb"
                        cpuvariant="Cortex-A57MPx"+str(self.options.numcores)
                        mpuflag="cortex-a57"
                        self.cpuarchitecture="ARM_CORTEX_A57"
                    elif self.options.architecture == architecturesE.cortexA72.name:
                        self.linuxdtb="foundation-v8-gicv3.dtb"
                        cpuvariant="Cortex-A72MPx"+str(self.options.numcores)
                        mpuflag="cortex-a72"
                        self.cpuarchitecture="ARM_CORTEX_A72"
                    else:
                        fimMessages().errorMessageWithTermination(message="Architecture not recognized")

                    # Update the simulator arguments
                    self.cmdarguments+=" --mode linux64 --linuxdtb {0} --startaddress 0x80000000".format(self.linuxdtb)

                    # Export environment variables to fill the makefiles
                    os.environ["ARM_TOOLCHAIN_LINUX"]="/usr"
                    os.environ["ARM_TOOLCHAIN_LINUX_PREFIX"]="aarch64-linux-gnu"

                #Flags to export to the Makefile
                self.dirnpb=""

                # Target workspace
                if   (self.options.workload==workloadsE.linux.name):
                    self.dirapplicationsource=self.dirworkload+"/linux"
                elif (self.options.workload==workloadsE.rodinia.name):
                    self.dirapplicationsource=self.dirworkload+"/rodinia"
                elif (self.options.workload==workloadsE.npbser.name):
                    self.dirapplicationsource=self.dirworkload+"/npbser"
                    self.dirnpb=self.dirsupport+"/npb-arm/NPB-SER"
                elif (self.options.workload==workloadsE.npbmpi.name):
                    self.dirapplicationsource=self.dirworkload+"/npbmpi"
                    self.dirnpb=self.dirsupport+"/npb-arm/NPB-MPI"
                elif (self.options.workload==workloadsE.npbomp.name):
                    self.dirapplicationsource=self.dirworkload+"/npbomp"
                    self.dirnpb=self.dirsupport+"/npb-arm/NPB-OMP"
                
                # Export environment variables to fill the makefiles
                os.environ["NPB_FOLDER"]=self.dirnpb
                os.environ["MPI_LIB"]=self.mpilib
                os.environ["MAKEFILE_CFLAGS"]="-O3 -g -w -gdwarf-2 -mcpu={0} -mlittle-endian -DUNIX -static -I{1} -D{2} -DOPEN -DNUM_THREAD=4 -fopenmp -pthread".format(mpuflag,self.dirfimapi,self.options.environment)
                os.environ["MAKEFILE_FLINKFLAGS"]="-static -fopenmp -I{0} -D{1} -lm -lstdc++".format(self.dirfimapi,self.options.environment)

                # Verify if the fimapi is compiled
                if( not os.path.isfile(self.dirfimapi+"/"+self.linuxfimapi)):
                    fimMessages().debugMessage(message="Recompiling the fim-api {0}:".format(self.linuxfimapi))
                    with open(self.dirfimapi+"/make.fimapi",'w') as stdout:
                        subprocess.check_call("make {0} -C {1}".format(self.options.environment,self.dirfimapi),shell=True,stdout=stdout,stderr=stdout);

                # common linux arguments
                self.cmdarguments+=" --verbose --variant {0} --symbolfile {1}  --symbolfile {2} --zimage {3} --initrd {4} --environment {5}"\
                        .format(cpuvariant,self.linuxvkernel,self.linuxfimapi,self.linuxkernel,self.linuximage,self.options.environment)

    # Compile the simulator and related components
    def compileSimulator(self):
        try:
            simstdout=open(self.dirworkspace+"/make.simulator","w")
            # Compiling the Platform and Intercept Library
            subprocess.check_call("make clean all VERBOSE=1 OPT=1 -C {0}".format(self.dirmodule)   ,shell=True,stdout=simstdout,stderr=simstdout)
            subprocess.check_call("make clean all VERBOSE=1 OPT=1 -C {0}".format(self.dirintercpet),shell=True,stdout=simstdout,stderr=simstdout)
            # retrocompatible environment variables
            os.environ["NODES"]="1"
            os.environ["PLATFORM_FOLDER"]=self.dirplatform
            subprocess.check_call("make clean all VERBOSE=1 OPT=1 -C {0}".format(self.dirharness)  ,shell=True,stdout=simstdout,stderr=simstdout)
        except:
            simstdout=open(self.dirworkspace+"/make.simulator","r")
            fimMessages().errorMessageWithTermination(message="Simulator Compilation Error:\n"+simstdout.read())
            print(simstdout.read())

    # Compile the current application
    def compileApplication(self):
        # Check if application exist
        if not os.path.exists(self.dirworkspace):
            fimMessages().errorMessageWithTermination(message="The application {0} source folder don't exist".format(self.currentapplication))

        # Current working folder
        self.dirworking=self.dirworkspace+"/"+self.currentapplication

        # Create the application working folder and fill
        subprocess.call("rm -rf "+self.dirworking,shell=True)
        subprocess.call("rsync -qav --exclude=\"{0}/{1}/Data\" {0}/{1} {2}".format(self.dirapplicationsource,self.currentapplication,self.dirworkspace),shell=True)
        os.chdir(self.dirworking)

        # Simulator executable
        shutil.copy(self.dirharness+"/"+self.cmdharness,self.dirworking)

        # Create the internal folders
        os.mkdir(self.dirworking+"/Dumps")
        os.mkdir(self.dirworking+"/Checkpoints")
        os.mkdir(self.dirworking+"/Reports")
        os.mkdir(self.dirworking+"/PlatformLogs")

        # Export environment variables to fill the makefiles
        os.environ["CROSS"]=self.cpuarchitecture
        
        # Redirect make output
        compilationlog=open(self.dirworking+"/make.application","w")

        if (self.options.workload==workloadsE.baremetal.name):
            fimMessages().errorMessageWithTermination(message="Baremetal not supported!")
        elif (self.options.workload==workloadsE.freertos.name):
            fimMessages().errorMessageWithTermination(message="FreeRTOS not supported!")
        elif ([ True for item in linuxworkloadsL if item== self.options.workload]):
            #Compile the application
            if not self.options.compilation == compilationOptsE.dontcompile.name:
                if ([ True for item in npbworkloadsL if item== self.options.workload]):
                    npbclass="CLASS=S"
                    os.environ["NPB_CLASS"]=npbclass
                    subprocess.call("cp -r {0}/callfim.c {1}".format(self.dirfimapi,self.dirworking),shell=True)
                    # Avoid old npb binaries
                    subprocess.call("rm {0}/common/*.o".format(self.dirnpb),shell=True)
                    # Compile
                    subprocess.check_call("make NPROCS={0} {1}".format(self.options.numcores,npbclass),shell=True,stdout=compilationlog,stderr=compilationlog)
                else:
                    subprocess.call("cp -r {0}/Makefile {1}".format(self.dirlinux,self.dirworking),shell=True)
                    try:
                        subprocess.check_call("make VERBOSE=1 -C {0}".format(self.dirworking),shell=True,stdout=compilationlog,stderr=compilationlog)
                    except:
                        fimMessages().errorMessageWithTermination(message="Application Compilation Error:\n"+compilationlog.read())
                        

            # Application executable file
            self.applicationobjfile=subprocess.check_output("find {0} -name \"*.elf\" -and ! -name \"smpboot.ARM_CORTEX_A9.elf\" ".format(self.dirworking),shell=True).rstrip("\n").split("/")[-1]
            
            # Optional arguments passed to the app
            applicationargs=""
            if os.path.exists(self.dirworking+"/arguments"):
                with open(self.dirworking+"/arguments",'r') as argfile:
                    applicationargs=argfile.read().rstrip("\n")

            #Copy the script and update the template
            subprocess.call("cp {0} {1}".format(self.dirlinux+"/"+self.linuxscript,self.dirworking),shell=True)

            #Application command into the caller scritp
            if (self.options.workload==workloadsE.npbmpi.name):
                if os.path.exists(self.dirworking+"/numThreads"):
                    with open(self.dirworking+"/numThreads",'r') as argfile:
                        mpianumthreads=argfile.read().rstrip("\n")
                    subprocess.call("sed -i \"s#APPLICATION=.*#APPLICATION=\\\"{0} -np {1} ./{2} {3}\\\"#\" {4}".format(self.mpicaller,mpianumthreads,self.applicationobjfile,applicationargs,self.dirworking+"/"+self.linuxscript),shell=True)
                else:
                    subprocess.call("sed -i \"s#APPLICATION=.*#APPLICATION=\\\"{0} -np {1} ./{2} {3}\\\"#\" {4}".format(self.mpicaller,self.options.numcores,self.applicationobjfile,applicationargs,self.dirworking+"/"+self.linuxscript),shell=True)
            else:
                subprocess.call("sed -i \"s#APPLICATION=.*#APPLICATION=\\\"./{0} {1}\\\"#\" {2}".format(self.applicationobjfile,applicationargs,self.dirworking+"/"+self.linuxscript),shell=True)

            #Application path inside the linux image 
            subprocess.call("sed -i \"s#APP_PATH=.*#APP_PATH=\\\"/{0}\\\"#\" {1}".format(self.linuxguestfolder+"/"+self.options.workload+"/"+self.currentapplication,self.dirworking+"/"+self.linuxscript),shell=True)

            #Simulator arguments specific for the app 
            self.cmdapp="  --projectfolder {0} --symbolfile {1} --applicationfolder {2}".format(self.dirproject,self.applicationobjfile,self.dirworking)

            # Copy the kernel and boot loader
            shutil.copy(self.dirlinuximages+"/"+self.linuxkernel,self.dirworking)
            shutil.copy(self.dirlinuximages+"/"+self.linuxvkernel,self.dirworking)
            shutil.copy(self.dirfimapi+"/"+self.linuxfimapi,self.dirworking)
            
            #DTB or bootloader
            if self.options.environment == environmentsE.ovparmv8.name:
                shutil.copy(self.dirlinuximages+"/"+self.linuxdtb,self.dirworking)
            else:
                shutil.copy(self.dirlinuximages+"/"+self.linuxbootloader,self.dirworking)

            #Create the linux image
            if not self.options.compilation == compilationOptsE.justcompile.name:
                self.createImage()
        
        # Return make.application
        return compilationlog
            #~ shutil.copy(self.dirlinuximages+"/"+self.linuximage,self.dirworking) ????

    # Create the linux image
    def createImage(self):
        #Temporary folder in temp
        tempdir=subprocess.check_output("mktemp -d",shell=True).rstrip("\n")

        #Source and dest image files
        linuximgsource = self.dirlinuximages +"/"+ self.linuxsourceimage
        linuximgdest   = self.dirworking     +"/"+ self.linuximage

        #Target linux root folder
        self.dirlinuxroot = tempdir+"/"+self.linuxguestfolder
        self.dirlinuxapp  = self.dirlinuxroot+"/"+self.options.workload+"/"+self.currentapplication

        #Unpack the linux image
        subprocess.check_call("{0}/utilities/unpackImage.sh {1} {2}".format(self.dirlinuximages,linuximgsource,tempdir),shell=True,stdout=self.FNULL,stderr=self.FNULL)

        # Create dir
        os.makedirs(self.dirlinuxapp)

        # Copy file
        shutil.copy(self.dirworking+"/"+self.applicationobjfile ,self.dirlinuxapp)
        shutil.copy(self.dirworking+"/"+self.linuxscript        ,self.dirlinuxroot)
        shutil.copy(self.dirworking+"/"+self.linuxfimapi        ,self.dirlinuxroot)

        if os.path.exists(self.dirapplicationsource+"/"+self.currentapplication+"/Data"):
            print(("cp -r {0} {1}".format(self.dirapplicationsource+"/"+self.currentapplication+"/Data",self.dirlinuxapp)))
            subprocess.check_call("cp -r {0} {1}".format(self.dirapplicationsource+"/"+self.currentapplication+"/Data",self.dirlinuxapp),shell=True)

        subprocess.check_call("chmod 777 -R "+tempdir,shell=True)

        # Pack the linux image
        subprocess.check_call("{0}/utilities/packImage.sh {1} {2}".format(self.dirlinuximages,tempdir,linuximgdest),shell=True,stdout=self.FNULL,stderr=self.FNULL)
        subprocess.call("rm -rf "+tempdir,shell=True)

    #
    # Fault Injection methods
    #
    # Create faults
    def createFaults(self,tracefile="trace.log"):
        # fault generator 
        faultgenerator = faultGenerator(options=self.options,goldinformation=self.dirworking+"/goldinformation",tracefile=tracefile)

        # create fault list
        faultlist = faultgenerator.createfaultlist()
    
    # Harvest the reports
    def harvest(self,exectime):
        harv = harvester(exectime=exectime,numfaults=self.options.numfaults,numcores=self.options.numcores,environment=self.options.environment,application=self.currentapplication)
        reportfile = harv.harvest()
        shutil.copy(reportfile ,self.dirworkspace)
        
    # fault injection traditional simulation Flow
    def run(self,command=""):
        #Configure the environment
        self.configuration()

        #Compile the simulator
        self.compileSimulator();
        
        # Acquire the applications
        if self.options.apps:
            applist = self.options.apps.split(",")
        else:
            applist = os.listdir(self.dirapplicationsource)
        
        applist.sort()
        applist = applist[:self.options.numapps]

        # Add extra command lines according with the command

        if ([ True for item in symbolfaultsL if item==self.options.faulttype]):
            cmdextra="--tracesymbol {0}".format(self.options.tracesymbol)
        else:
            cmdextra=""

        # Main loop iteration
        for currentapplication in applist:
            self.currentapplication=currentapplication
            self.writeTerminal("Beginning application: {0}".format(self.currentapplication))
            
            # Compile the application
            self.compileApplication()

            # Reference Phase
            simcommnad="{0} {1} {2} --numberoffaults 1 {3}".format(self.cmdharness,self.cmdarguments,self.cmdapp,cmdextra)
            
            start = time.time()
            thread = subprocess.Popen( shlex.split("./{0} --platformid 0 --goldrun".format(simcommnad)),
                stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr = subprocess.PIPE, shell = False)
            threading.Thread(target=self.terminalScroll, args=(thread,)).start()
            thread.wait()
            # Gold Execution
            goldexectime=time.time()-start

            # Create Faults
            self.createFaults()
            
            # Fault Injection Tag
            self.writeTerminal("Fault Injection Starting : {:%Y-%b-%d %H:%M:%S} \n".format(datetime.datetime.now()))
            
            # Create a pool of working threads
            self.obj = mapping(self.options,self.dirworking,simcommnad,goldexectime)
            simulationtime = self.obj.run()
            
            # Fault Injection End Tag
            self.writeTerminal("Fault Injection Finished after {0:.3} seconds\n".format(simulationtime))
            self.harvest(exectime=int(simulationtime*1E9))
            
            # Display results
            with open(glob.glob("{0}/*.reportfile".format(self.dirworking))[0]) as f:
                self.writeTerminal(f.read())

            # Back to the root folder
            os.chdir(self.dirproject)

        # Close queue
        self.exitCleaning()

    #
    # Profiling methods
    #
    # Function Profiling
    def profile(self,command,profile):
        # Configure the environment
        self.configuration()

        # Compile the simulator
        self.compileSimulator();
        
        # Current Appllication
        self.currentapplication=self.options.apps

        self.writeTerminal("Beginning {0} on the application: {1}".format(profile,self.currentapplication))

        # Compile the application
        log = self.compileApplication()
        
        # Extra arguments to the simulator
        cmdextra = ""
        
        # Return
        if   command == "compile":
            # Display makefile log
            with open(self.dirworking+"/make.application") as f:
                self.writeTerminal(f.read())
            self.exitCleaning()
            return
        # Add extra command lines according with the command
        elif command == "symbol":
            cmdextra="--enabletools --{0}".format(profile,self.options.tracesymbol)

        thread = subprocess.Popen( shlex.split("./{0} {1} {2} --platformid 0 --numberoffaults 1 --goldrun --enableconsole {3}".format(self.cmdharness,self.cmdarguments,self.cmdapp,cmdextra)),
            stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr = subprocess.PIPE, shell = False)
        threading.Thread(target=self.terminalScroll, args=(thread,)).start()
        thread.wait()

        #Create output
        if command == "symbol":
            if   profile == "enablefunctionprofile":
                output = subprocess.check_output("ipost.exe --name app -f *.iprof", shell = True)
            elif profile == "enablelinecoverage":
                output = subprocess.check_output("ipost.exe --name app -f *.lcov", shell = True)
            self.writeTerminal(output)
                
        # Close queue
        self.exitCleaning()

#
# Special class to call the multiprocessing.Pool. The map function serializes the context which is impossible because of other calls, thus this class encapsulates the context.
#
class mapping():
    def __init__(self,options,dirworking,simcommnad,goldexectime):
        self.options      = options
        self.dirworking   = dirworking
        self.simcommnad   = simcommnad
        self.goldexectime = goldexectime

    # Fault injection working process
    def faultInjectionThread(self,faultindex):
        timeoutexectime = self.goldexectime * 1.5 + 55

        with open(self.dirworking+"/PlatformLogs/platform-"+str(faultindex),'w') as stdout:
            try:
                subprocess.check_call("timeout {0} ./{1} --platformid {2} --faultcampaignrun"
                    .format(timeoutexectime,self.simcommnad,faultindex),shell=True,stdout=stdout,stderr=stdout)
            except:
                print("Timeout thread {0}.".format(faultindex))
                #~ try:
                    #~ subprocess.check_call("timeout {0} ./{1} --platformid {2} --faultcampaignrun"
                        #~ .format(timeoutexectime,self.simcommnad,faultindex),shell=True,stdout=stdout,stderr=stdout)
                #~ except:
                    #~ print "Timeout thread {0}".format(faultindex)

    # Terminate the process
    def terminate(self):
        self.pool.terminate()

    # Initiator
    def run(self):
        start = time.time()
        pool = multiprocessing.Pool(processes=self.options.parallel)
        pool.map(self.faultInjectionThread,list(range(1,self.options.numfaults+1)))
        pool.close()
        pool.join()
        return time.time()-start


