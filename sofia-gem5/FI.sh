#!/bin/bash
#
#PBS -N Gem5Sim-createjobs
#PBS -l walltime=4:00:00
#PBS -l pvmem=6gb
#PBS -l nodes=1:ppn=1
#PBS -m bea
#PBS -p 1023
#PBS -M feliperochadarosa@gmail.com

: '
"""
Fault Injection Module to Gem5
Version 3.0
14/02/2017

Author: Felipe Rocha da Rosa - frdarosa.com

Annotations:
while :; do telnet 127.0.0.1 3456 2>/dev/null ; sleep 1; done
--debug-flags=Exec

read -p "Press [Enter] key to start..."
ghex ./m5out/gold_execution/system.physmem.store0.pmem
--elastic-trace-en --data-trace-file=deptrace.proto.gz --inst-trace-file=fetchtrace.proto.gz
"""
'
    ############################### Sub Functions ##################################################
    #Create the fault list
    function generateFaultList {
        #Common arguments
        CMD_FAULT_LIST="$CMD_FAULT_LIST --environment $ENVIRONMENT --numberoffaults $NUMBER_OF_FAULTS --faulttype $FAULT_TYPE"

        # register list
        if [[ ! -z $REG_LIST ]]; then
            CMD_FAULT_LIST="$CMD_FAULT_LIST --reglist $REG_LIST"
        fi

        # not generating faults
        if [[ $GENERATE_FAULT_LIST -eq $NO_FAULTS ]]; then
            CMD_FAULT_LIST="$CMD_FAULT_LIST --nobitflip"
            GENERATE_FAULT_LIST_DISTR=0 #Don't necessary
        fi

        # generating the same number of faults per register
        if [[ $GENERATE_FAULT_LIST_DISTR -eq 1 ]]; then
            CMD_FAULT_LIST="$CMD_FAULT_LIST --eqdist"
        fi

        # run the faultlist
        $CMD_FAULT_LIST --memsize=0x40000000 --memaddress=0x00000000

        # check if app make was successful, if not exits the script
        if [ $? -ne 0 ]; then exit 1; fi
    }

    # create the checkpoints
    function generateCheckpoints {
        #Get the number of executed instructions
        INSTRUCTIONS_GOLD=$(grep "Application # of instructions" ./goldinformation | cut -d '=' -f2)

        if [[ "$NUMBER_CHKP" -ne 0 ]]; then
            #Calculating the checkpoint size
            if [[ "$NUM_CORES" -gt 1 ]]; then
                #~ CHECKPOINT_INTERVAL=$((TicksGold/NUMBER_CHKP))
                CHECKPOINT_INTERVAL=$((INSTRUCTIONS_GOLD/NUMBER_CHKP))
            else
                CHECKPOINT_INTERVAL=$((INSTRUCTIONS_GOLD/NUMBER_CHKP))
            fi
            echo "Checkpoint interval" $CHECKPOINT_INTERVAL
        fi

        if [[ "$CHECKPOINT_INTERVAL" -ne 0 ]]; then

            echo "**************************************************************"
            echo -e "\e[93mCreate the checkpoints\e[0m"
            echo "**************************************************************"
            export M5OUTFILES="m5outfiles/gold" # change/create the Gem5 work folder to enable parallel executions
            CMD_SIM=$(echo "$CMD_GEM5 $SIM_ARGS" | envsubst);
            #First run in atomic mode
            $CMD_SIM --goldrun \
                     --cpu-type atomic \
                     --restore-with-cpu $CPU_MODEL \
                     --checkpointinterval $CHECKPOINT_INTERVAL \
                     > ./PlatformLogs/goldcheckpoint 2>&1

            #Check if the gold run was sucessfull, if not exits the script
            if [ $? -ne 0 ]; then exit 1; fi

                echo "**************************************************************"
                echo -e "\e[93mExtract extra information \e[0m"
                echo "**************************************************************"

                for ((i=1; i <= $(find . -name "FIM.Profile.*" | wc -l) ; i=i+1))
                do
                    echo "**************************************************************"
                    echo -e "\e[31mPlatform  \e[0m"$i
                    echo "**************************************************************"
                    export M5OUTFILES="$WORKING_FOLDER/m5outfiles/$i" #Change/Create the Gem5 work folder to enable parallel executions
                    CMD_SIM=$(echo "$CMD_GEM5 $SIM_ARGS" | envsubst);
                    $CMD_SIM --goldrun \
                             --platformid $i \
                             --cpu-type $CPU_MODEL \
                             > $WORKING_FOLDER/PlatformLogs/checkpoint-$i 2>&1 &

                    parallelExecutionManager
                done
                wait

        fi
    }

    # compile the application and setup the simulation args
    function compileApplication {
        # Current working folder
        export WORKING_FOLDER="$WORKSPACE/$CURRENT_APPLICATION"

        #Check if application exist
        if [ ! -d $CURRENT_APPLICATION ]; then
            echo "Application $CURRENT_APPLICATION does not recognized for this workload" && exit
        fi

        # updated the application folder in the workspace
        rm -rf  $WORKING_FOLDER
        rsync -qavR --exclude="$CURRENT_APPLICATION/Data" $CURRENT_APPLICATION $WORKSPACE

        # enter in the application folder in the workspace
        cd $WORKING_FOLDER

        #create infrastructure folders
        mkdir -p ./Reports ./PlatformLogs

        #New variable for arguments
        SIM_ARGS=" --applicationfolder \$WORKING_FOLDER"

        #In some makefile is definied CROSS
        export CROSS=$ARCHITECTURE

        # remove old elfs
        if [[ $LI_OPTIONS -eq $LI_USE_CLEAN ]]; then rm *elf ; fi

        #Compile the application according the workload type
        case $WORKLOAD_TYPE in
        WORKLOAD_BAREMETAL)
            echo "Baremetal application"
            if [[ $LI_OPTIONS -ne $LI_DONT_COMPILE ]]; then
                #Copying the makefile and selecting the architecture
                cp -f $BAREMETAL_FOLDER/Makefile .
                make clean all VERBOSE=1 &>make.application

                if [ $? -ne 0 ]; then cat make.application; exit 1; fi #Check if app make was successful, if not exits the script
            fi

            #Application Name
            APPLICATIONS_NAME=$(find -name "*.elf" | cut -c 3-)
            #Optional arguments
            APPLICATIONS_ARGS=$(<arguments)

            #Prepare command line
            SIM_ARGS="$SIM_ARGS --kernel \$WORKING_FOLDER/$APPLICATIONS_NAME"
            #~ if [ -f "arguments" ]; then CMD_SIM="$CMD_SIM --applicationargs $APPLICATIONS_ARGS"; fi
        ;; #end baremetal
        WORKLOAD_NPB_MPI | WORKLOAD_NPB_OMP | WORKLOAD_NPB_SER | WORKLOAD_RODINIA | WORKLOAD_LINUX | WORKLOAD_MIBENCH | WORKLOAD_WCET)
            echo "Linux Based"

            #Compile the application
            if [[ $LI_OPTIONS -ne $LI_DONT_COMPILE ]]; then
            if [[ $WORKLOAD_TYPE -eq $WORKLOAD_NPB_OMP ]] || [[ $WORKLOAD_TYPE -eq $WORKLOAD_NPB_SER ]] || [[ $WORKLOAD_TYPE -eq $WORKLOAD_NPB_MPI ]]; then
                export NPB_CLASS="CLASS=S"
                cp -f $OVP_FIM/callfim.c .
                #avoid old binaries
                rm $NPB_FOLDER/common/*.o  &>make.application

                make NPROCS=$NUM_CORES $NPB_CLASS &>make.application
                #Check if app make was successful, if not exits the script
                if [ $? -ne 0 ]; then cat make.application; exit 1; fi
            else
                cp -f $LINUX_FOLDER/Makefile .
                make VERBOSE=1 &>make.application
                #Check if app make was successful, if not exits the script
                if [ $? -ne 0 ]; then cat make.application; exit 1; fi
            fi
            fi

            #Call the application into the gem5 command
            SIM_ARGS="$SIM_ARGS --script \$WORKING_FOLDER/$LINUX_RUNSCRIPT"

            #Application Name
            APPLICATIONS_NAME=$(find -name "*.elf" -and ! -name "smpboot.ARM_CORTEX_A9.elf" | cut -c 3-)
            APPLICATIONS_ARGS=$(<arguments)

            #Copy the script and update the template
            cp -f $LINUX_FOLDER/$LINUX_RUNSCRIPT .

            case $WORKLOAD_TYPE in
            WORKLOAD_NPB_MPI) #add the mpirun
            NUM_MPI_THREADS=$(<numThreads)
            if [[ -z $NUM_MPI_THREADS ]]; then
                echo "$APPLICATIONS_NAME $APPLICATIONS_ARGS"
                sed -i "s#APPLICATION=.*#APPLICATION=\"$MPICALLER -np $NUM_CORES ./$APPLICATIONS_NAME $APPLICATIONS_ARGS\" #" $LINUX_RUNSCRIPT
            else
                sed -i "s#APPLICATION=.*#APPLICATION=\"$MPICALLER -np $NUM_MPI_THREADS ./$APPLICATIONS_NAME $APPLICATIONS_ARGS\" #" $LINUX_RUNSCRIPT
            fi ;;
            WORKLOAD_NPB_OMP | WORKLOAD_NPB_SER | WORKLOAD_RODINIA | WORKLOAD_LINUX | WORKLOAD_MIBENCH | WORKLOAD_WCET )  sed -i "s#APPLICATION=.*#APPLICATION=\"./$APPLICATIONS_NAME $APPLICATIONS_ARGS\" #" $LINUX_RUNSCRIPT;;
            *) echo "Invalid Workload!"; exit ;;
            esac

            #Application path inside the linux image
            sed -i "s#APP_PATH=.*#APP_PATH=\"/$LINUX_HOME_ROOT/$WORKLOAD_TYPE/$CURRENT_APPLICATION\" #" $LINUX_RUNSCRIPT

            #mount the image and copy the current benchmark
            if [[ $LI_OPTIONS -ne $LI_DONT_COMPILE ]]; then
                createImage $WORKLOAD_TYPE
            fi
        ;; #end linux
        WORKLOAD_CASE_STUDY)
            echo "Case Study"
            #Compile the application
            if [[ $LI_OPTIONS -ne $LI_DONT_COMPILE ]]; then

                if [[ "$CURRENT_APPLICATION" == "libviso" ]]; then
                    echo "Case Study ! $CURRENT_APPLICATION --"

                    # zlib
                    cd zlib-1.2.11
                        TARGETMACH=arm-none-linux-gnueabi      \
                        BUILDMACH=x86_64-pc-linux-gnu          \
                        CC=${CS_TOOLCHAIN_HF}-gcc              \
                        LD=${CS_TOOLCHAIN_HF}-ld               \
                        AS=${CS_TOOLCHAIN_HF}-as               \
                        CFLAGS=$CS_CFLAGS                      \
                        ./configure --prefix=$PWD --static

                        make -j install
                    cd -
                    # libpng
                    cd libpng-1.6.34
                        CC=${CS_TOOLCHAIN_HF}-gcc              \
                        CXX=${CS_TOOLCHAIN_HF}-g++             \
                        LD=${CS_TOOLCHAIN_HF}-ld               \
                        AS=${CS_TOOLCHAIN_HF}-as               \
                        CFLAGS=$CS_CFLAGS                      \
                        LDFLAGS='-L../zlib-1.2.11/lib/'        \
                        ./configure \
                            --build=arm-linux-gnueabi --target=arm-linux-gnueabi --host=arm-linux-gnueabihf  \
                            --prefix=$PWD --enable-static --enable-arm-neon=api

                        make -j install
                    cd -
                    #libviso2
                    CC=${CS_TOOLCHAIN_HF}-gcc       \
                    CPP=${CS_TOOLCHAIN_HF}-g++      \
                    CXX=${CS_TOOLCHAIN_HF}-g++      \
                    LD=${CS_TOOLCHAIN_HF}-ld        \
                    AS=${CS_TOOLCHAIN_HF}-as        \
                    CFLAGS="$CS_CFLAGS -D$ENVIRONMENT"     \
                    cmake -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON libviso2

                    make

                elif [[ "$CURRENT_APPLICATION" == "darknet-yolov2" ]]; then
                    echo "Case Study ! $CURRENT_APPLICATION"
                    make -j #&>make.application

                    #Check if app make was successful, if not exits the script
                    if [ $? -ne 0 ]; then cat make.application; exit 1; fi

                    # Create the elf
                    cp darknet darknet.elf

                else
                    echo "Case Study ! This application is not configured $CURRENT_APPLICATION"
                    exit
                fi # Case studies
            fi

            #Call the application into the gem5 command
            SIM_ARGS="$SIM_ARGS --script \$WORKING_FOLDER/$LINUX_RUNSCRIPT"

            #Application Name
            APPLICATIONS_NAME=$(find -name "*.elf" -and ! -name "smpboot.ARM_CORTEX_A9.elf" | cut -c 3-)
            APPLICATIONS_ARGS=$(<arguments)

            #Copy the script and update the template
            cp -f $LINUX_FOLDER/$LINUX_RUNSCRIPT .
            sed -i "s#APPLICATION=.*#APPLICATION=\"./$APPLICATIONS_NAME $APPLICATIONS_ARGS\" #" $LINUX_RUNSCRIPT

            # application path inside the linux image
            sed -i "s#APP_PATH=.*#APP_PATH=\"/$LINUX_HOME_ROOT/$WORKLOAD_TYPE/$CURRENT_APPLICATION\" #" $LINUX_RUNSCRIPT

            #mount the image and copy the current benchmark
            if [[ $LI_OPTIONS -ne $LI_DONT_COMPILE ]]; then
                createImage $WORKLOAD_TYPE
            fi
        ;; # End Case Study
        esac

        #Copy back the application elf
        rm -f $APPLICATIONS_FOLDER/$CURRENT_APPLICATION/*.elf
        cp -f $APPLICATIONS_NAME $APPLICATIONS_FOLDER/$CURRENT_APPLICATION/

        #Next application if LI_JUST_COMPILE
        if [[ $LI_OPTIONS -eq $LI_JUST_COMPILE ]]; then
            echo "LI_JUST_COMPILE"
            folderCounter
            continue
        fi
    }

    # create the linux image
    function createImage {
        #Temporary folder in temp
        TEMP_DIR=$(mktemp -d)

        #Copy linux image @Review better way to always copy
        if [[ $LI_OPTIONS -eq "$LI_USE_CLEAN" ]]; then
            echo "Create temporary image 1"
            cp -f $LINUX_IMAGE_ORIGINAL $LINUX_IMAGE
        elif ([[ $LI_OPTIONS -eq "$LI_APPEND" ]] || [[ $LI_OPTIONS -eq "$LI_JUST_COMPILE" ]]) && [[ ! -f $LINUX_IMAGE ]]; then
            echo "Create temporary image 2"
            cp -f $LINUX_IMAGE_ORIGINAL $LINUX_IMAGE
        fi

        #Mount the linux image
        sudo mount -o loop,offset=32256 $LINUX_IMAGE $TEMP_DIR

        #Copy to the disk image
        sudo mkdir -p                    $TEMP_DIR/$LINUX_HOME_ROOT/$1/$CURRENT_APPLICATION
        sudo cp -f $APPLICATIONS_NAME    $TEMP_DIR/$LINUX_HOME_ROOT/$1/$CURRENT_APPLICATION
        sudo cp -f $LINUX_FOLDER/m5/m5   $TEMP_DIR/$LINUX_HOME_ROOT/

        if [[ "$CURRENT_APPLICATION" == "darknet-yolov2" ]]; then
            echo "darknet"
            sudo cp -r cfg data $TEMP_DIR/$LINUX_HOME_ROOT/$1/$CURRENT_APPLICATION
        else
            sudo cp -r $APPLICATIONS_FOLDER/$CURRENT_APPLICATION/Data $TEMP_DIR/$LINUX_HOME_ROOT/$1/$CURRENT_APPLICATION
        fi
        
        #Change permissions
        sudo chmod 777 -R $TEMP_DIR/$LINUX_HOME_ROOT/*

        #Unmount and clean
        sudo umount $TEMP_DIR
        sudo rm -rf $TEMP_DIR
    }

    #Create the checkpoints
    function checkpointKernel {
        NUM_CORES_IN_THE_KERNEL_CHECKPOINT=$(<$LINUX_KERNEL_CHECKPOINT/NUM_CORES_IN_THIS_KERNEL)

        #Create the kernel boot first checkpoint that will be used to all applications
        if ([ ! -d "$LINUX_KERNEL_CHECKPOINT" ] || [[ "$NUM_CORES_IN_THE_KERNEL_CHECKPOINT" -ne "$NUM_CORES" ]]) && [[ "$MODE" -eq $FIM_FS_MODE ]]; then
            # Cannot create inside the alice; no sudo
            if [[ ! -z $ALICE_HPC ]]; then
                echo "Cannot create the kernel checkpoint inside the alice; no sudo" && exit
            fi

            #Note see /etc/rc.d/rc.local to get the first checkpoint just before the script call
            echo "**************************************************************"
            echo -e "\e[92mLinux Kernel checkpoint \e[0m"
            echo "**************************************************************"

            if [[ "$NUM_CORES_IN_THE_KERNEL_CHECKPOINT" -ne "$NUM_CORES" ]]; then
                rm -rf $LINUX_KERNEL_CHECKPOINT
            fi

            #Create the kernel folder
            mkdir $LINUX_KERNEL_CHECKPOINT
            cd $LINUX_KERNEL_CHECKPOINT

            #Temporary folder
            TEMP_DIR=$(mktemp -d)

            #Copy linux imagem
            cp -f $LINUX_IMAGE_ORIGINAL $LINUX_IMAGE

            #Mount the linux image
            sudo mount -o loop,offset=32256 $LINUX_IMAGE $TEMP_DIR

            #Clean the application folder
            sudo rm -rf $TEMP_DIR/$LINUX_HOME_ROOT/*

            #Copy the file to the image
            sudo cp -f $LINUX_FOLDER/m5/m5 $TEMP_DIR/$LINUX_HOME_ROOT/

            #Make permissions
            sudo chmod 777 -R $TEMP_DIR/$LINUX_HOME_ROOT/*

            #Unmount and clean
            sudo umount $TEMP_DIR
            sudo rm -rf $TEMP_DIR

            #Time to unmount partition
            sleep 5

            CMD_SIM=$(echo "$CMD_GEM5 $SIM_ARGS" | envsubst);
            #Create the kernel checkpoint
            $CMD_SIM    --goldrun \
                        --kernelcheckpointafterboot \
                        --cpu-type atomic

            mv ./m5out*/gold/FIM.After_Kernel_Boot* .
            echo $NUM_CORES > NUM_CORES_IN_THIS_KERNEL

            cd $APPLICATIONS_FOLDER
        fi
    }

    # configure the simulation commands and environment
    function configureCommands {
        # define paths
        # root folder - All paths will be derived from here automatically
        if [[ -z $PBS_O_WORKDIR ]]; then #Running inside a job
            export PROJECT_FOLDER=$(pwd)
        else
            export PROJECT_FOLDER=$PBS_O_WORKDIR
        fi

        #Working space folder
        export WORKSPACE=$PROJECT_FOLDER/workspace

        #Workloads folder
        export WORKLOADS=$PROJECT_FOLDER/workloads

        #Support tools and files
        export SUPPORT_FOLDER=$PROJECT_FOLDER/support

        #Folder containing the Gem5 Simulator
        export GEM5_FOLDER=$PROJECT_FOLDER/gem5-source

        # Gem5 simulator call command
        export GEM5_COMMAND=build/ARM/gem5.fast

        #Folder containing the support tools
        export TOOLS_FOLDER=$SUPPORT_FOLDER/tools

        #Python Version
        export PYTHON_V="python"

        #Fault generator command
        export CMD_FAULT_LIST="$PYTHON_V $TOOLS_FOLDER/faultList.py"

        #Fault harvest command
        export CMD_FAULT_HARVEST="$PYTHON_V $TOOLS_FOLDER/harvest.py"

        #Profile command
        export CMD_PROFILE="$PYTHON_V $TOOLS_FOLDER/timeout.py"

        #Custom Time
        export CTIME="/usr/bin/time -f \"\t%E real,\t%U user,\t%S sys,\t%M mem\" "

        #Suffix for temporary files and folders
        export SUFIX_FIM_TEMP_FILES=FIM_log

        #OVP_FIM Library
        export OVP_FIM=$SUPPORT_FOLDER/fim-api

        # enumerators
        source $SUPPORT_FOLDER/variables.sh

        # create the workspace folder
        mkdir -p $WORKSPACE

        if [[ -z $ALICE_HPC ]]; then #Not inside ALICE HPC
            MAX_NPROC=$(($(nproc)/2)) #default, get the number of processing units available
        else #Number of cores in the alice HPC flow (not number of jobs)
            if [[ -z $PBS_NUM_PPN ]]; then  #Running inside the login node
                MAX_NPROC=16
            else                            #Running inside a job
                MAX_NPROC=$PBS_NUM_PPN
            fi
        fi

        # Force some options
        # Dont compile the applications
        if [[ ! -z $ALICE_HPC ]]; then
            LI_OPTIONS=LI_DONT_COMPILE
            module unload gcc/6.1
            module load   gcc/5.4
        fi
        
        # Compatible gcc
        export CC="gcc-5"
        export CXX="g++-5"

        #Queue variables
        NUM=0
        QUEUE=""

        #Simulator dependent defines and variables
        #CPU types
        case $CPU_TYPE in
        ATOMIC)   CPU_MODEL="atomic";;
        TIMING)   CPU_MODEL="timing";;
        DETAILED) CPU_MODEL="detailed";;
        *) echo "Invalid CPU!"; exit ;;
        esac

        #Default m5outfiles
        export M5OUTFILES="m5outfiles/gold"

        # gem5 commands template
        CMD_GEM5_FS="\$GEM5_FOLDER/$GEM5_COMMAND --outdir=\$M5OUTFILES \$GEM5_FOLDER/src/fim/fs-fim.py --projectfolder $PROJECT_FOLDER $cachesConfig $memConfig --fullsystem --kernelcheckpoint \$LINUX_KERNEL_CHECKPOINT --disk-image \$LINUX_IMAGE"
        CMD_GEM5_SE="\$GEM5_FOLDER/build/ARM/gem5.opt --outdir=\$M5OUTFILES \$GEM5_FOLDER/src/fim/FIM_se.py --projectfolder $PROJECT_FOLDER $cachesConfig $memConfig"

        # create the command string to call the Gem5 simulator
        case $MODE in
        FIM_SE_MODE) CMD_GEM5="$CMD_GEM5_SE"; echo "Not supported" && exit;;
        FIM_FS_MODE) CMD_GEM5="$CMD_GEM5_FS -n $NUM_CORES";;
        FIM_BA_MODE)
            #~ memConfig="--mem-size=256MB" #Required
            WORKLOAD_TYPE=WORKLOAD_BAREMETAL
            CMD_GEM5="$CMD_GEM5_FS --bare-metal --machine-type=RealView_PBX --dtb-filename=None" ;;
        *) echo "Invalid Mode!"; exit ;;
        esac
        # Workload definition
        case $WORKLOAD_TYPE in
        WORKLOAD_BAREMETAL)
        # Baremetal Applications, ignore the multi-core factor

            #Bare-metal support folder
            export BAREMETAL_FOLDER=$SUPPORT_FOLDER/baremetal

            #CrossCompiler
            export ARM_TOOLCHAIN_BAREMETAL=/usr
            export ARM_TOOLCHAIN_BAREMETAL_PREFIX=arm-none-eabi

            #CrossCompiler Baremetal AARCH32
            export ARM_TOOLCHAIN_BAREMETAL64=/usr
            export ARM_TOOLCHAIN_BAREMETAL64_PREFIX=arm-none-eabi

            # applications source
            APPLICATIONS_FOLDER=$WORKLOADS/baremetal

            #Flags to export to the Makefile
            export MAKEFILE_CFLAGS="-g -w -I$SUPPORT_FOLDER -I$OVP_FIM -D$ENVIRONMENT -DBAREMETAL -std=gnu99 -O3"

        ;; #end baremetal

        WORKLOAD_LINUX)
            case $ARCHITECTURE in
            ARM_CORTEX_A9) #For the moment only A9 linux
                # environment
                ENVIRONMENT=gem5armv7
                #MPI library
                export MPI_LIB=$SUPPORT_FOLDER/mpich-3.2-armv7                                      # mpi library path
                export MPICALLER=armv7-linux-gnueabi-mpirun                                         # mpi app caller

                export LINUX_FOLDER=$SUPPORT_FOLDER/linux-armv7                                         # linux support folder
                export LINUX_IMAGES_FOLDER=$LINUX_FOLDER/disks                                          # folder with the full-system images
                export LINUX_IMAGE_ORIGINAL=$LINUX_IMAGES_FOLDER/linux-aarch32-ael.img                  # linux  image
                export LINUX_IMAGE=$LINUX_IMAGE_ORIGINAL.$WORKLOAD_TYPE.$ENVIRONMENT.$NUM_CORES.$SUFIX_FIM_TEMP_FILES   # loaded image - temporary -> no git track
                export LINUX_KERNEL_CHECKPOINT=$LINUX_FOLDER/kernelCheckpoint"-"$NUM_CORES              # kernel checkpoint
                export LINUX_KERNEL=$LINUX_FOLDER/binaries/vmlinux4.3                                   # kernel binary
                export LINUX_VKERNEL=                                                                   # kernel binary
                export LINUX_BOOTLOADER=                                                                # bootloader binary
                export LINUX_HOME_ROOT=root                                                             #  target linux root folder
                export LINUX_RUNSCRIPT=Script.rcS                                                       # script responsible to call the application inside the linux
                export M5_PATH=$LINUX_FOLDER                                                            # M5 path - required by the Full System mode
                export M5_LINK_LIB="-lm5"                                                               # M5 Libm5.a to link

                #CrossCompiler Linux
                export ARM_TOOLCHAIN_LINUX=/usr
                export ARM_TOOLCHAIN_LINUX_PREFIX=arm-linux-gnueabi

                CPU_VARIANT=Cortex-A9MPx$NUM_CORES
                MPUFLAG="cortex-a9"

                # define the workload path
                case $WORKLOAD_TYPE in
                WORKLOAD_LINUX)   APPLICATIONS_FOLDER=$WORKLOADS/linux;;
                *) echo "Invalid Workload!"; exit ;;
                esac

                #Arch-depend argunments
                #~ CMD_GEM5+=" --machine-type=VExpress_EMM --environment=$ENVIRONMENT" # Old Kernel
                CMD_GEM5+=" --machine-type=VExpress_GEM5_V1 --environment=$ENVIRONMENT --dtb=$LINUX_FOLDER/binaries/armv7_gem5_v1_"$NUM_CORES"cpu.dtb --kernel $LINUX_KERNEL"

                ;; #End ARM_CORTEX_A9
            ARM_CORTEX_A53 | ARM_CORTEX_A57 | ARM_CORTEX_A72)
                # environment
                ENVIRONMENT=gem5armv8

                #MPI library
                export MPI_LIB=$SUPPORT_FOLDER/mpich-3.2-armv8                                      # mpi library path
                export MPICALLER=armv8-linux-gnueabi-mpirun                                         # mpi app caller

                export LINUX_FOLDER=$SUPPORT_FOLDER/linux-armv8                                         # linux support folder
                export LINUX_IMAGES_FOLDER=$LINUX_FOLDER/disks                                          # folder with the full-system images
                export LINUX_IMAGE_ORIGINAL=$LINUX_IMAGES_FOLDER/aarch64-ubuntu-trusty-headless.img     # linux  image
                export LINUX_IMAGE=$LINUX_IMAGE_ORIGINAL.$WORKLOAD_TYPE.$ENVIRONMENT.$NUM_CORES.$SUFIX_FIM_TEMP_FILES   # loaded image - temporary -> no git track
                export LINUX_KERNEL_CHECKPOINT=$LINUX_FOLDER/kernelCheckpoint"-"$NUM_CORES              # kernel checkpoint
                export LINUX_KERNEL=$LINUX_FOLDER/binaries/vmlinux                                      # kernel binary
                export LINUX_VKERNEL=                                                                   # kernel binary
                export LINUX_BOOTLOADER=                                                                # bootloader binary
                export LINUX_HOME_ROOT=home/root                                                        # target linux root folder
                export LINUX_RUNSCRIPT=Script.rcS                                                       # script responsible to call the application inside the linux
                export M5_PATH=$LINUX_FOLDER                                                            # M5 path - required by the Full System mode
                export M5_LINK_LIB="-lm5"                                                               # M5 Libm5.a to link

                #CrossCompiler Linux
                export ARM_TOOLCHAIN_LINUX=/usr
                export ARM_TOOLCHAIN_LINUX_PREFIX=aarch64-linux-gnu

                CPU_VARIANT=Cortex-72MPx$NUM_CORES
                MPUFLAG="cortex-a72"

                # define the workload path
                case $WORKLOAD_TYPE in
                WORKLOAD_LINUX)   APPLICATIONS_FOLDER=$WORKLOADS/linux;;
				*) echo "Invalid Workload!"; exit ;;
                esac

                #Arch-depend argunments
                CMD_GEM5+=" --machine-type=VExpress_GEM5_V1 --dtb=$LINUX_FOLDER/binaries/armv8_gem5_v1_"$NUM_CORES"cpu.dtb --environment=$ENVIRONMENT --kernel $LINUX_KERNEL"

                ;; #End ARMv8

            ARM_V8_BL_2_2 | ARM_V8_BL_2_4)
                # environment
                ENVIRONMENT=gem5armv8

                #MPI library
                export MPI_LIB=$SUPPORT_FOLDER/mpich-3.2-armv8                                      # mpi library path
                export MPICALLER=armv8-linux-gnueabi-mpirun                                         # mpi app caller

                export LINUX_FOLDER=$SUPPORT_FOLDER/linux-armv8                                         # linux support folder
                export LINUX_IMAGES_FOLDER=$LINUX_FOLDER/disks                                          # folder with the full-system images
                export LINUX_IMAGE_ORIGINAL=$LINUX_IMAGES_FOLDER/aarch64-ubuntu-trusty-headless.img     # linux  image
                export LINUX_IMAGE=$LINUX_IMAGE_ORIGINAL.$WORKLOAD_TYPE.$ENVIRONMENT.$NUM_CORES.$SUFIX_FIM_TEMP_FILES   # loaded image - temporary -> no git track
                export LINUX_KERNEL_CHECKPOINT=$LINUX_FOLDER/kernelCheckpoint"-"$ARCHITECTURE           # kernel checkpoint
                export LINUX_KERNEL=$LINUX_FOLDER/binaries/vmlinux                                      # kernel binary
                export LINUX_VKERNEL=                                                                   # kernel binary
                export LINUX_BOOTLOADER=                                                                # bootloader binary
                export LINUX_HOME_ROOT=home/root                                                        # target linux root folder
                export LINUX_RUNSCRIPT=Script.rcS                                                       # script responsible to call the application inside the linux
                export M5_PATH=$LINUX_FOLDER                                                            # M5 path - required by the Full System mode
                export M5_LINK_LIB="-lm5"                                                               # M5 Libm5.a to link

                #CrossCompiler Linux
                export ARM_TOOLCHAIN_LINUX=/usr
                export ARM_TOOLCHAIN_LINUX_PREFIX=aarch64-linux-gnu

                CPU_VARIANT=Cortex-A53MPx$NUM_CORES
                MPUFLAG="cortex-a53"

                # define the workload path
                case $WORKLOAD_TYPE in
                WORKLOAD_LINUX)   APPLICATIONS_FOLDER=$WORKLOADS/linux;;
                *) echo "Invalid Workload!"; exit ;;
                esac

                #Arch-depend argunments
                if [ $ARCHITECTURE = 'ARM_V8_BL_2_2' ]; then
                    CMD_GEM5+=" --machine-type=VExpress_GEM5_V1 --dtb=armv8_gem5_v1_big_little_2_2.20170616.dtb --environment=$ENVIRONMENT --kernel $LINUX_KERNEL"
                else
                    CMD_GEM5+=" --machine-type=VExpress_GEM5_V1 --dtb=armv8_gem5_v1_big_little_2_4.20170616.dtb --environment=$ENVIRONMENT --kernel $LINUX_KERNEL"
                fi
                ;; #End ARMv8 bigLITTLE
            *) echo "Invalid Workload processor match"; exit ;;
            esac

            #Flags to export to the Makefile
            export MAKEFILE_CFLAGS="-O3 -g -w -gdwarf-2 -mcpu=$MPUFLAG -mlittle-endian -DUNIX -static -L$LINUX_FOLDER/m5 -I$LINUX_FOLDER/m5 -I$OVP_FIM -D$ENVIRONMENT -DOPEN -DNUM_THREAD=4 -fopenmp -pthread -lm -lstdc++ -lm5"
            export MAKEFILE_FLINKFLAGS="-static -fopenmp -L$LINUX_FOLDER/m5 -I$OVP_FIM -D$ENVIRONMENT -lm -lstdc++ -lm5"
            export MAKEFILE_LIBRARIES=" -fopenmp -pthread -lm -lstdc++ -lm5"

        ;; #End Linux
        *) echo "Invalid Workload!"; exit ;;
        esac

        ### SYNC POINT ####
        # count the number of visited folders
        COUNTER=1

        # enter in the folder containing the applications
        cd $APPLICATIONS_FOLDER

        #Get the folders in the target application root folder
        if [[ -z $APPLICATION_LIST ]]; then
            APPLICATION_LIST=$(ls)
        fi

        #Test if the ENVIRONMENT was set
        if [[ -z $ENVIRONMENT ]]; then
            echo "ENVIRONMENT not set!" && exit
        fi
    }

    # submit jobs to the ALICE HPC; Divide the NUMBER_OF_FAULTS by NUM_PARALLEL_JOBS
    function submitJobs {
            #QSUB binary
            export QSUB="qsub"
            GOLD_EXEC=$(  echo "$GOLD_EXEC + 45" | bc) # add the comparison time!

            LC_NUMERIC="en_US.UTF-8"
            # Number of days to execute the workload
            EXECTIME=$( printf "%.0f" $(echo "scale=3;(($GOLD_EXEC*$NUMBER_OF_FAULTS)*1.075 + 1000)/(3600*24)" | bc) )

            #Search the smaller divisor$
            for i in $(seq 1 $NUMBER_OF_FAULTS); do
                DIVISOR=$(echo "$NUMBER_OF_FAULTS % $i" | bc)

                # Let pass only divisors from this number
                if (( $DIVISOR!=0 )); then continue; fi
                if (( $i > $EXECTIME )); then NUM_PARALLEL_JOBS=$i && break; fi
            done

            #~ NUM_PARALLEL_JOBS=10
            # calculate the thread workload and variable sizes
            WINDOW_SIZE=$(echo "$NUMBER_OF_FAULTS / $NUM_PARALLEL_JOBS" | bc)
            ITERATIONS=$( echo "$NUMBER_OF_FAULTS / $WINDOW_SIZE" | bc)
            #WALLTIME=$(   echo "$GOLD_EXEC * $WINDOW_SIZE * 1.075 + 1000"| bc) # add the moving files to local node time (worst case to 250 parallel threads) plus a extra time for hanged platforms!
            WALLTIME=$(   echo "24 * 3600"| bc) #Get 24h jobs
            WALLTIME_H=$( echo "$WALLTIME / 3600" | bc )
            WALLTIME_M=$( echo "($WALLTIME - ($WALLTIME_H*3600))/60" | bc)
            WALLTIME_S=$( echo "$WALLTIME  - ($WALLTIME_H*3600) -($WALLTIME_M*60)" | bc)
            WALL=${WALLTIME_H%.*}":"${WALLTIME_M%.*}":"${WALLTIME_S%.*}
            echo "$CURRENT_APPLICATION WALLTIME:"$WALL" JOBS :"$NUM_PARALLEL_JOBS" GOLD EXEC (s) :"$GOLD_EXEC

            BASE=1
            TOP=$WINDOW_SIZE
            DEPENDCY=

            # create the compressed file
            time 7za -m0=lzma2 -mx9 a -mmt=$MAX_NPROC -t7z $WORKSPACE/$CURRENT_APPLICATION.7z \
            $LINUX_FOLDER/binaries \
            $LINUX_IMAGE \
            $LINUX_KERNEL_CHECKPOINT \
            $GEM5_FOLDER \
            $TOOLS_FOLDER/classification.py \
            $WORKSPACE/$CURRENT_APPLICATION/*


            CMD_SIM="$SIM_CALLER $CMD_GEM5 $SIM_ARGS --cpu-type $CPU_MODEL --faultcampaignrun --numberoffaults $FAULTS_PER_PLATFORM --checkpointinterval $CHECKPOINT_INTERVAL --platformid \$i"
            CPCMD="$CTIME rsync -acl --bwlimit=15000 -e \"ssh -o ConnectTimeout=2 -o ServerAliveInterval=2 -ServerAliveCountMax=2\""
#           CHECK="\n if [ \$? -ne 0 ]; then echo \"Aborting...\"; qdel \$PBS_JOBID && qsub $WORKSPACE/$CURRENT_APPLICATION/JOB.\$i.sh; fi"
            CHECK="\n if [ \$? -ne 0 ]; then echo \"Aborting(\$SECONDS)...\"; qdel \$PBS_JOBID; fi"
            for ((i=1; i <= "${ITERATIONS}" ; i=i+1))
            do
                echo "**************************************************************"
                echo -e "\e[31mDispatch the job $i array from $BASE to $TOP \e[0m"
                echo "**************************************************************"
                JOB_NAME="$CURRENT_APPLICATION-C$NUM_CORES-$CPU_TYPE-$i-$USER-$WORKLOAD_TYPE"

                echo -e "#!/bin/bash"\
                "\n#PBS -N      $JOB_NAME      "\
                "\n#PBS -l      walltime=$WALL "\
                "\n#PBS -l      pvmem=6gb      "\
                "\n#PBS -l      nodes=1:ppn=1  "\
                "\n#PBS -M      feliperochadarosa@gmail.com -ma

                export          WORKING_FOLDER=\$TMPDIR
                export          LINUX_IMAGE=\$TMPDIR/${LINUX_IMAGE##*/}
                export          LINUX_KERNEL_CHECKPOINT=\$TMPDIR/${LINUX_KERNEL_CHECKPOINT##*/}
                export          GEM5_FOLDER=\$TMPDIR/gem5-source
                export          M5_PATH=\$TMPDIR

                chmod 755 -R \$TMPDIR
                cd \$TMPDIR
                cp     -a     $WORKSPACE/$CURRENT_APPLICATION.7z      \$TMPDIR    $CHECK
                echo \"Copying time \$SECONDS\"
                chmod 755 -R *

                $CTIME 7z -mmt=1 x  $CURRENT_APPLICATION.7z     $CHECK
                echo \"Decompress time \$SECONDS\"
                chmod 755 -R *

                for ((i=$BASE; i <= $TOP ; i=i+1)); do
                    export M5OUTFILES=\$TMPDIR/m5outfiles/\$i
                    echo \"**************************************************************\"
                    echo \"Beginning Platform \$i \"
                    echo \"**************************************************************\"

                    $CTIME $CMD_SIM
                    if [ \$? -eq 124 ]; then echo \"Aborting TIMEOUT!!!!\"; fi

                    rm -rf $WORKING_FOLDER/m5outfiles/\$i
                    cp -f ./Reports/reportfile-\$i $WORKSPACE/$CURRENT_APPLICATION/Reports/
                done

                cp -f ./Reports/* $WORKSPACE/$CURRENT_APPLICATION/Reports/

                echo \" #Executed Time: \$SECONDS\"

                " > JOB.$i.sh

                #LAST_JOB=$($QSUB  JOB.$i.sh -a $(date -d "today + $((($i/50)*180 + 30)) seconds" +'%m%d%H%M.%S'))
                #~ LAST_JOB=$($QSUB  JOB.$i.sh )

                echo "SUB JOB "$LAST_JOB
                DEPENDCY="$DEPENDCY:${LAST_JOB%.*.*.*}"

                # recalculate the window interval
                BASE=$(($BASE+$WINDOW_SIZE))
                TOP=$(($TOP+$WINDOW_SIZE))
            done

            echo "**************************************************************"
            echo -e "\e[31mDispatch the harvest job \e[0m"
            echo "**************************************************************"
            JOB_NAME="$CURRENT_APPLICATION-C$NUM_CORES-$CPU_TYPE-$USER-$WORKLOAD_TYPE-RESULTS"
            echo -e "#!/bin/bash"\
            "\n#PBS -N       $JOB_NAME              "\
            "\n#PBS -l       walltime=00:30:00      "\
            "\n#PBS -l       pvmem=1gb              "\
            "\n#PBS -l       nodes=1:ppn=1          "\
            "\n#PBS -M       feliperochadarosa@gmail.com -mea

            cd $WORKING_FOLDER
            $CMD_FAULT_HARVEST --parallelexecutions $NUMBER_OF_FAULTS --numberoffaults $FAULTS_PER_PLATFORM --application  $CURRENT_APPLICATION --cores $NUM_CORES --environment $ENVIRONMENT --cputype $CPU_TYPE
            cp -f *reportfile $WORKSPACE" > JOB-HARVEST.sh

            #~ JOB_HARVEST=$($QSUB -W depend=afterok$DEPENDCY JOB-HARVEST.sh)

            #exit after N folders
            folderCounter
            continue
    }

    ################################################################################################

    # entry time stamp
    entertime="$(date +%s%N)"

    # clean the terminal
    #~ reset

    ############################### simulation variables ###########################################
: '
    This is the chosen architecture use in the cross-compiler
    makefile and use as filename extension
'
    ARCHITECTURE=ARM_CORTEX_A72

: '
    Number of TARGET cores
'
    NUM_CORES=1
: '
    Number of NUMBER_OF_FAULTS that equals to the number of injected faults
'
    NUMBER_OF_FAULTS=10
    FAULTS_PER_PLATFORM=1

: '
    Generate a new fault list
    NO_GENERATION   - Do not generate a new fault list
    GENERATE_FAULTS - Generate faults
    NO_FAULTS       - Generate faults with no bit flips
'
    GENERATE_FAULT_LIST=GENERATE_FAULTS

: '
    The use of checkpoints will be defined by this two variables. If one of then is not equal to zero,
    all the flow will automatically adjust to use checkpoints. The first define the number of checkpoints
    according with the number of executed instructions in the gold run, thus defining the checkpoint interval.
    Additionally, is also possible define directly the interval.
'
    NUMBER_CHKP=0
    CHECKPOINT_INTERVAL=0

: '
    Registers to create/inject faults - leave empty for all registers
'
    REG_LIST=

: '
    Fault Type choice
    register        -> Register file
    robsource       -> Reorder Buffer Source register
    robdest         -> Reorder Buffer destination register
    variable        -> Trace and inject fault in a memory region
    functiontrace   -> Function trace
'
    FAULT_TYPE=register

    # enables the same number of faults per register
    GENERATE_FAULT_LIST_DISTR=0

: '
    Workload selection
    WORKLOAD_BAREMETAL  - baremetal
    WORKLOAD_LINUX      - Linux

'
    WORKLOAD_TYPE=WORKLOAD_LINUX

: '
    #Number of folders (1 = first folder)
    #list of applications, if empty all application are selected
'
    VISITED_FOLDERS=100
    APPLICATION_LIST="$@"

: '
    Linux Image source and options
    LI_USE_CLEAN        -> use the clean version and replace the temporary image   |[SUDO REQUIRED]|
    LI_APPEND           -> append to the temporary image [Create one if not exist] |[SUDO REQUIRED]|
    LI_DONT_COMPILE     -> Use the image available [Use in server without sudo access]
    LI_JUST_COMPILE     -> Compile all applications and create the image [Without RUNNING]
'
    LI_OPTIONS=LI_USE_CLEAN

: '
    MODE
    FIM_SE_MODE -> Syscall
    FIM_FS_MODE -> FullSystem
    FIM_BA_MODE -> Bare Metal
'
    MODE=FIM_FS_MODE

: '
    Cpu Model choice
    ATOMIC   -> atomic
    TIMING   -> timing --need review
    DETAILED -> DerivO3CPU
'
    CPU_TYPE=ATOMIC

: '
    Memory and Cache configuration - If you change this values will be necessary recompile the linux boot checkpoint
    by erasing the folder in Support/Linux_Support
'
    cachesConfig="--caches --l1i_size=32kB --l1i_assoc=4 --l1d_size=32kB --l1d_assoc=4 --l2_size=512kB --l2_assoc=8"
    memConfig="--mem-size=1024MB --mem-type SimpleMemory"

    # configure the simulator command according with the options
    configureCommands

    ################################################################################################
    ############################### Fault Campaign Began ###########################################
    ################################################################################################
    echo "**************************************************************"
    echo -e "\e[31mCompiling the Gem5 \e[0m"
    echo "**************************************************************"
    GEM5_RECOMPILE=0
    if [[ ! -d $GEM5_FOLDER/build || $GEM5_RECOMPILE -eq 1 ]]; then # Check if the Gem5/build folder exist before build
        if [ -x "$(command -v gcc-5)" ]; then
            alias g++=g++-5
            alias gcc=gcc-5
        else
            echo "Please instal the gcc 5 -> sudo apt install gcc-5 g++-5"
            exit
        fi

        if [[ -z $ALICE_HPC ]]; then
            $PYTHON_V `which scons` --directory=$GEM5_FOLDER -s -j $(nproc) $GEM5_FOLDER/$GEM5_COMMAND
        else #HPC FLOW
            cd $GEM5_FOLDER
                $PYTHON_V `which scons` -s -j 12 ./$GEM5_COMMAND \
                &&  echo "Gem5 successfully compiled!"
                exit
            cd -
        fi
    fi

    # check if make was successful, if not exits the script
    if [ $? -ne 0 ]; then exit 1; fi

    # execute the kernel and checkpoint at the end of the Linux boot --once
    checkpointKernel

    # accessing each application folder
    for applicationFolders in $APPLICATION_LIST
    do
        #remove the last slash
        CURRENT_APPLICATION=${applicationFolders%*/}

        echo "**************************************************************"
        echo -e "\e[31mApplication beginning:\e[0m" $CURRENT_APPLICATION
        echo "**************************************************************"
        echo $CURRENT_APPLICATION "$(date +"%T"\ "%D")" >> $WORKSPACE/applications

        time="$(($(date +%s%N)-entertime))"
        echo $CURRENT_APPLICATION "$((time/1000000000))" >> $WORKSPACE/timing
            # clean the folder and compile the application
            compileApplication
            echo "**************************************************************"
            echo -e "\e[31mBeginning Gold Execution\e[0m"
            echo "**************************************************************"
            export M5OUTFILES="m5outfiles/gold" # change/create the Gem5 work folder to enable parallel executions
            CMD_SIM=$(echo "$CMD_GEM5 $SIM_ARGS" | envsubst);

            # run the gold platform
            GOLD_EXEC=$SECONDS

            $CMD_SIM --cpu-type $CPU_MODEL --goldrun \
            #~ > $WORKING_FOLDER/PlatformLogs/gold 2>&1

            # check if the gold run was successful, if not exits the script
            if [ $? -ne 0 ]; then exit 1; fi
            GOLD_EXEC=$(($SECONDS - $GOLD_EXEC))

            # create the checkpoints when required
            generateCheckpoints

            # generating a fault list
            generateFaultList

            echo "**************************************************************"
            echo -e "\e[31mBeginning Fault Injection Campaign \e[0m"
            echo "**************************************************************"

            time="$(($(date +%s%N)-entertime))"
            echo "Fault_Injection $((time/1000000000))" >> $WORKSPACE/timing

            # time stamp for the fault injection
            sFaultCampaignBegin="$(date +%s%N)"

            SIM_CALLER="timeout "$((GOLD_EXEC*2+150))

            if [[ -z $ALICE_HPC ]]; then #Normal flow
            for ((i=1; i <= "${NUMBER_OF_FAULTS}" ; i=i+1))
            do
                echo "**************************************************************"
                echo -e "\e[31mBeginning Platform $i \e[0m"
                echo "**************************************************************"
                export M5OUTFILES="$WORKING_FOLDER/m5outfiles/$i" # change/create the Gem5 work folder to enable parallel executions
                CMD_SIM="$SIM_CALLER $(echo "$CMD_GEM5 $SIM_ARGS" | envsubst) --cpu-type $CPU_MODEL --faultcampaignrun --numberoffaults $FAULTS_PER_PLATFORM --checkpointinterval $CHECKPOINT_INTERVAL --platformid $i"

                # execute the FI platform
                /usr/bin/time -f "Measures, $CURRENT_APPLICATION, \t%E real,\t%U user,\t%S sys,\t%M mem" $CMD_SIM > $WORKING_FOLDER/PlatformLogs/platform-$i 2>&1 &

               parallelExecutionManager
            done
            else #HPC FLOW
                submitJobs
            fi

            wait # wait for all processes to finish before exit

            # time interval in nanoseconds
            nsFaultCampaignBegin="$(($(date +%s%N)-sFaultCampaignBegin))"
            # milliseconds
            msFaultCampaignBegin="$((nsFaultCampaignBegin/1000000))"
            # seconds
            sFaultCampaignBegin="$((nsFaultCampaignBegin/1000000000))"


            echo "**************************************************************"
            echo "Simulation time in  nanoseconds: ${nsFaultCampaignBegin}"
            echo "Simulation time in milliseconds: ${msFaultCampaignBegin}"
            echo "Simulation time in      seconds: ${sFaultCampaignBegin}"
            echo "**************************************************************"
            echo "Fault harvest"
            echo "**************************************************************"

            $CMD_FAULT_HARVEST  --parallelexecutions $NUMBER_OF_FAULTS \
                                --numberoffaults     $FAULTS_PER_PLATFORM \
                                --application        $CURRENT_APPLICATION \
                                --executiontime      $nsFaultCampaignBegin \
                                --cores              $NUM_CORES \
                                --cputype            $CPU_TYPE \
                                --environment        $ENVIRONMENT

            # ending
            cat     *reportfile
            cp -f   *reportfile $WORKSPACE

            # exit after N folders
            #Return to the applications folder
            cd $APPLICATIONS_FOLDER
            if [[ "$COUNTER" -eq "$VISITED_FOLDERS" ]]; then
                break
            else
                COUNTER=$((COUNTER+1))
            fi

    done #for dir

    #back to the root folder
    cd ..

    echo "**************************************************************"
    echo "Simulation end"
    echo "**************************************************************"
    time="$(($(date +%s%N)-entertime))"
    echo "Simulation_End" "$((time/1000000000))" >> $WORKSPACE/timing
