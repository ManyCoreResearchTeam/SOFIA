#!/bin/bash

#set -euo

: '
"""
Fault Injection Module to OVP
Version 4.2 beta
2022-08-05

Authors:

Felipe Rocha da Rosa - frdarosa.com
Geancarlo Abich - abich@ieee.org
Jonas Gava - jfgava@inf.ufrgs.br
Isadora Oliveira - isoliveira@inf.ufrgs.br
Vitor Bandeira - bandeira@ieee.org
Nicolas Lodéa - nicolaslodea@gmail.com

"""
'

function callHarvestLocal {
        # Get elapsed time for all simulations/campaigns
        # Time interval in nanoseconds
        nsFaultCampaignBegin="$(($(date +%s%N)-sFaultCampaignBegin))"
        # Milliseconds
        msFaultCampaignBegin="$((nsFaultCampaignBegin/1000000))"
        # Seconds
        sFaultCampaignBegin="$((nsFaultCampaignBegin/1000000000))"

        echo "**************************************************************"
        echo "Simulation time in nanoseconds: ${nsFaultCampaignBegin}"
        echo "Simulation time in milliseconds: ${msFaultCampaignBegin}"
        echo "Simulation time in seconds: ${sFaultCampaignBegin}"
        echo "**************************************************************"
        echo "Fault harvest"
        echo "**************************************************************"

        # Call fault harvest to generate final report file
        $CMD_FAULT_HARVEST --parallelexecutions "$NUMBER_OF_FAULTS" \
                           --numberoffaults "$FAULTS_PER_PLATFORM" \
                           --application "$CURRENT_APPLICATION" \
                           --executiontime "$nsFaultCampaignBegin" \
                           --cores "$NUM_CORES" --cputype "$CPU_TYPE" \
                           --environment "$ENVIRONMENT"

        $CMD_FAULT_GRAPHIC --replotfile $CURRENT_APPLICATION.${ENVIRONMENT}.reportfile \
                           --groupsdac

}

function generateFaultList {
        # Common arguments
        CMD_FAULT_LIST="$CMD_FAULT_LIST --environment $ENVIRONMENT --numberoffaults $NUMBER_OF_FAULTS --faulttype $FAULT_TYPE"
        # Register list
        if [[ -n "$REG_LIST" ]]; then
                CMD_FAULT_LIST="$CMD_FAULT_LIST --reglist $REG_LIST"
        fi
        # Not generating faults
        if [[ $GENERATE_FAULT_LIST -eq $NO_FAULTS ]]; then
                CMD_FAULT_LIST="$CMD_FAULT_LIST --nobitflip"
                GENERATE_FAULT_LIST_DISTR=0 # Don't necessary
        fi
        # Not generating faults =register
        if [[ "$FAULT_TYPE" == "register" ]]; then
                CMD_FAULT_LIST="$CMD_FAULT_LIST --memsize=$TARGET_MEM_SIZE --memaddress=$TARGET_MEM_BASE"
                GENERATE_FAULT_LIST_DISTR=0 # Don't necessary
        fi
        # Generating the same number of faults per register
        if [[ "$GENERATE_FAULT_LIST_DISTR" -eq 1 ]]; then
                CMD_FAULT_LIST="$CMD_FAULT_LIST --eqdist"
        fi
        # Run the faultlist
        # Check if app make was successful, if not exits the script
        if [[ "$("$CMD_FAULT_LIST")" -ne 0 ]]; then exit 1; fi
}

function generateFaultList2 {
        # Common arguments
        CMD_FAULT_LIST="$PYTHON_V $TOOLS_FOLDER/faultList2.py --environment $ENVIRONMENT --numfaults $NUMBER_OF_FAULTS --faulttype $FAULT_TYPE --bitflips $NUMBER_OF_BITFLIPS"
        # Register list
        if [[ -n "$REG_LIST" ]]; then
                CMD_FAULT_LIST="$CMD_FAULT_LIST --registerlist $REG_LIST"
        fi
        if [[ -n "$FUNCTION_INSTANCE" ]]; then
                CMD_FAULT_LIST="$CMD_FAULT_LIST --traceinstance $FUNCTION_INSTANCE"
        fi
        # Not generating faults
        if [[ "$GENERATE_FAULT_LIST" -eq "$NO_FAULTS" ]]; then
                CMD_FAULT_LIST="$CMD_FAULT_LIST --dummy"
                GENERATE_FAULT_LIST_DISTR=0 # Don't necessary
        fi
        # Not generating faults =register
        if [[ "$FAULT_TYPE" == "memory" ]]; then
                CMD_FAULT_LIST="$CMD_FAULT_LIST --memlowaddress=$TARGET_MEM_BASE --memhighaddress=$TARGET_MEM_SIZE"
                GENERATE_FAULT_LIST_DISTR=0 # Don't necessary
        fi

        # Virtual Address memory
        if [ $FAULT_TYPE == "memory2" ]; then
            #old version
            #EntryAddres=$(grep "\.text" applicationSections | sed 's/.*PROGBITS//' | awk '{ print $1  }')
            #FinalAddres=$(grep "\.bss" applicationSections | sed 's/.*NOBITS//' | awk '{ print $1  }')
            #FinalSize=$(grep "\.bss" applicationSections | sed 's/.*NOBITS//' | awk '{ print $2  }')
            #MEM_LOW=$(echo "ibase=16;obase=A; $(echo "$EntryAddres" | tr "[:lower:]" "[:upper:]")" | bc )
            #MEM_HIGH=$(echo "ibase=16;obase=A; $(echo "$FinalAddres+$FinalSize" | tr "[:lower:]" "[:upper:]")" | bc )
            #CMD_FAULT_LIST="$CMD_FAULT_LIST --memlowaddress=$MEM_LOW --memhighaddress=$MEM_HIGH"
            # Get virtual address range
            arm-linux-gnueabi-readelf -S $APPLICATIONS_NAME > applicationSections
            # Flash Memory entry address
            EntryAddress=$( A=$(grep "\.text" applicationSections) ; echo "${A#*PROGBITS}" | tr -s ' ' | cut -d " " -f2 )
            # Allocated Flash Memory
            FlashSize=$( A=$(grep "\.text" applicationSections) ; echo "${A#*PROGBITS}" | tr -s ' ' | cut -d " " -f4 )
            # RAM entry address
            RAMDataAddress=$( A=$(grep "\.data" applicationSections) ; echo "${A#*PROGBITS}" | tr -s ' ' | cut -d " " -f2 )
            # DATA Allocated RAM
            RAMDataSize=$( A=$(grep "\.data" applicationSections) ; echo "${A#*PROGBITS}" | tr -s ' ' | cut -d " " -f4 )
            # RAM BSS entry address
            RAMBSSAddress=$( A=$(grep "\.bss"  applicationSections) ; echo "${A#*NOBITS}"   | tr -s ' ' | cut -d " " -f2 )
            # BSS Allocated RAM
            RAMBSSSize=$( A=$(grep "\.bss"  applicationSections) ; echo "${A#*NOBITS}"   | tr -s ' ' | cut -d " " -f4 )
            # RAM Final Address
            RAMEnd=$( A=$(grep "\.heap" applicationSections) ; echo "${A#*PROGBITS}" | tr -s ' ' | cut -d " " -f2 )
            # Output to memory detailing file
            echo "$CURRENT_APPLICATION Memory Detailement on $ARCHITECTURE (Sizes are in bytes)" &>> memory_details.log
            echo "application, flash_start, flash_end, ram_start, ram_end, flash_size, data_size, bss_size, total_ram_size" &>> memory_details.log
            echo "$CURRENT_APPLICATION, $EntryAddress, $FlashSize, $RAMDataAddress, $RAMEnd, $(  echo "ibase=16;obase=A; $( echo "$FlashSize" | tr "a-z" "A-Z")" | bc ), $(  echo "ibase=16;obase=A; $( echo "$RAMDataSize" | tr "a-z" "A-Z")" | bc ), $(  echo "ibase=16;obase=A; $( echo "$RAMBSSSize" | tr "a-z" "A-Z")" | bc ), $(  echo "ibase=16;obase=A; $( echo "$RAMDataSize+$RAMBSSSize" | tr "a-z" "A-Z")" | bc )" &>> memory_details.log
            if [ -z $MEMORY_OPTIONS ]; then
                CMD_FAULT_LIST="$CMD_FAULT_LIST --memlowaddress=$(  echo "ibase=16;obase=A; $( echo "$EntryAddress"        | tr "a-z" "A-Z")" | bc ) \
                                                --memhighaddress=$( echo "ibase=16;obase=A; $( echo "$RAMEnd"             | tr "a-z" "A-Z")" | bc )"
            else
                if [ $MEMORY_OPTIONS == "FLASH" ]; then
                    CMD_FAULT_LIST="$CMD_FAULT_LIST --memlowaddress=$(  echo "ibase=16;obase=A; $( echo "$EntryAddress"    | tr "a-z" "A-Z")" | bc ) \
                                                    --memhighaddress=$( echo "ibase=16;obase=A; $( echo "$FlashSize"      | tr "a-z" "A-Z")" | bc )"
                fi
                if [ $MEMORY_OPTIONS == "RAM" ]; then
                    CMD_FAULT_LIST="$CMD_FAULT_LIST --memlowaddress=$(  echo "ibase=16;obase=A; $( echo "$RAMDataAddress"  | tr "a-z" "A-Z")" | bc ) \
                                                    --memhighaddress=$( echo "ibase=16;obase=A; $( echo "$RAMEnd"         | tr "a-z" "A-Z")" | bc )"
                fi
            fi

            GENERATE_FAULT_LIST_DISTR=0 #Don't necessary
        fi

        # Generating the same number of faults per register
        if [[ "$GENERATE_FAULT_LIST_DISTR" -eq 1 ]]; then
                CMD_FAULT_LIST="$CMD_FAULT_LIST --eqdist"
        fi
	# Flip sequential bits
	if [[ "$SEQUENTIAL_BIT_FLIP" -eq 1 ]]; then
		CMD_FAULT_LIST="$CMD_FAULT_LIST --seqflip"
	fi

        # Not generating new faults
        if [[ "$GENERATE_FAULT_LIST" -ne "$NO_GENERATION" ]]; then
                # Run the faultlist
                # Check if app make was successful, if not exits the script
                $CMD_FAULT_LIST || exit 1
        fi
}

# create the checkpoints
function generateCheckpoints {
        # Get the number of executed instructions
        if [[ ! -f goldinformation ]]; then
                echo "No goldinformation file"
                exit 1
        fi
        INSTRUCTIONS_GOLD=$(grep -i "application # of instructions" ./goldinformation | cut -d '=' -f2)
        if [[ "$NUMBER_CHKP" -ne 0 ]]; then
                # Calculating the checkpoint size
                CHECKPOINT_INTERVAL=$((INSTRUCTIONS_GOLD/NUMBER_CHKP))
                echo "Checkpoint interval $CHECKPOINT_INTERVAL"
        fi
        if [[ "$CHECKPOINT_INTERVAL" -ne 0 ]]; then
                echo "**************************************************************"
                echo -e "\e[93mCreate the checkpoints\e[0m"
                echo "**************************************************************"
                # Calculating the checkpoint size
                echo "Checkpoint interval $CHECKPOINT_INTERVAL"
                # Create the checkpoints
                "$PLATFORM_FOLDER/harness/$CMD_OVP" "$SIM_ARGS" --goldrun --checkpointinterval "$CHECKPOINT_INTERVAL" --numberoffaults 1 --platformid 0 --applicationfolder "$WORKING_FOLDER" &> "./PlatformLogs/checkpoint_platform.$SUFIX_FIM_TEMP_FILES"
        fi
}

# compile the application and setup the simulation args
function compileApplication {
        # Current working folder
        export WORKING_FOLDER="$WORKSPACE/$CURRENT_APPLICATION"
        # Check if application exist
        if [[ ! -d "$CURRENT_APPLICATION" ]]; then
                echo "Application $CURRENT_APPLICATION does not recognized for this workload" && exit
        fi
        # Updated the application folder in the workspace
        rm -rf "$WORKING_FOLDER"
        rsync -qavR --exclude="$CURRENT_APPLICATION/Data" "$CURRENT_APPLICATION" "$WORKSPACE"
        # Enter in the application folder in the workspace
        cd "$WORKING_FOLDER" || exit
        # Copy the simulator executable and linux
        cp -rf "$PLATFORM_FOLDER/harness/harness.$IMPERAS_ARCH.exe" .

        # Create infrastructure folders
        mkdir -p ./Dumps ./Checkpoints ./Reports ./PlatformLogs ./Profiling
        # New variable for arguments
        SIM_ARGS=
        # In some makefile is definied CROSS
        export CROSS=$ARCHITECTURE
        # Remove old elfs
        if [[ "$LI_OPTIONS" -eq "$LI_USE_CLEAN" ]]; then
                rm -f ./*elf
        fi

        # Compile the application according the workload type
        case $WORKLOAD_TYPE in

                WORKLOAD_BAREMETAL)
                        echo "Baremetal application"
                        if [[ "$LI_OPTIONS" -ne "$LI_DONT_COMPILE" ]]; then
                                # Copying the makefile and selecting the architecture
                                cp -rf "$BAREMETAL_FOLDER/Makefile" .
                                # Check if app make was successful, if not exits the script
                                $(make clean all VERBOSE=1 &>make.application)
                                if [[ ! -f "app.$ARCHITECTURE.elf" ]]; then
                                        cat make.application
                                        exit 1
                                fi
                        fi
                        # Application Name
                        APPLICATIONS_NAME=$(find . -name "*.elf" | cut -c 3-)
                        # Optional arguments
                        if [[ -f arguments ]]; then
                                APPLICATIONS_ARGS=$(<arguments)
                        fi

                        # Prepare command line
                        SIM_ARGS="$SIM_ARGS --program $APPLICATIONS_NAME"
                        # if [[ -f "arguments" ]]; then CMD_SIM="$CMD_SIM --applicationargs $APPLICATIONS_ARGS"; fi
                        ;; # End baremetal

                WORKLOAD_LINUX)
                        # Copy the kernel and boot loader
                        cp -rf "$LINUX_IMAGES_FOLDER/$LINUX_KERNEL" .
                        cp -rf "$LINUX_IMAGES_FOLDER/$LINUX_BOOTLOADER" .
                        cp -rf "$LINUX_IMAGES_FOLDER/$LINUX_KERNEL" "$WORKING_FOLDER"
                        cp -rf "$LINUX_IMAGES_FOLDER/$LINUX_VKERNEL" "$WORKING_FOLDER"
                        cp -rf "$LINUX_IMAGES_FOLDER/$LINUX_BOOTLOADER" "$WORKING_FOLDER"
                        if [[ "$ARCHITECTURE" = "ARM_CORTEX_A72" ]]; then
                        	cp -rf "$LINUX_IMAGES_FOLDER/$LINUX_DTB" "$WORKING_FOLDER"
                        fi
                        # Compile the application
                        if [[ "$LI_OPTIONS" -ne "$LI_DONT_COMPILE" ]]; then
								cp -rf "$LINUX_FOLDER/Makefile" .
								# Check if app make was successful, if not exits the script
								$(make VERBOSE=1 &>make.application)
                                if [[ ! -f "app.$ARCHITECTURE.elf" ]]; then
                                        cat make.application
                                        exit 1
                                fi
                        fi
                        # Application Name
                        APPLICATIONS_NAME=$(find . -name "*.elf" -and ! -name "smpboot.ARM_CORTEX_A9.elf" | cut -c 3-)
                        if [[ -f arguments ]]; then
                                APPLICATIONS_ARGS=$(<arguments)
                        else
                                APPLICATIONS_ARGS=
                        fi
                       
                        # Copy the script and update the template
                        cp -rf "$LINUX_FOLDER/$LINUX_RUNSCRIPT" .
						sed -i "s#APPLICATION=.*#APPLICATION=\"./$APPLICATIONS_NAME $APPLICATIONS_ARGS\" #" "$LINUX_RUNSCRIPT"
                         
                        # Application path inside the linux image
                        sed -i "s#APP_PATH=.*#APP_PATH=\"/$LINUX_HOME_ROOT/$WORKLOAD_TYPE/$CURRENT_APPLICATION\" #" "$LINUX_RUNSCRIPT"
                        # Image dump name
                        # sed -i "s#DUMP_PATH=.*#DUMP_PATH=\"/$WORKSPACE/$CURRENT_APPLICATION/Dumps/inverted_image.pgm\" #" "$LINUX_RUNSCRIPT"
                        # Mount the image and copy the current benchmark
                        if [[ "$LI_OPTIONS" -ne "$LI_JUST_COMPILE" ]]; then
                                createImage "$WORKLOAD_TYPE"
                        fi
                        ;; # End linux
        esac

        # Copy back the application elf
        rm -f "$APPLICATIONS_FOLDER/$CURRENT_APPLICATION/*.elf"
        if [[ "$APPLICATIONS_NAME" ]]; then
                cp -rf "$APPLICATIONS_NAME" "$APPLICATIONS_FOLDER/$CURRENT_APPLICATION/"
        fi

        # Original project folder and application symbol list
        SIM_ARGS="$SIM_ARGS --projectfolder $PROJECT_FOLDER --symbolfile $APPLICATIONS_NAME"

}

# create the linux image
function createImage {
        # Temporary folder in temp
        TEMP_DIR=$(mktemp -d 2>/dev/null)

        # Source and dest image files
        SOURCE_IMG=$LINUX_IMAGES_FOLDER/$LINUX_IMAGE_ORIGINAL
        # DEST_IMG=$LINUX_IMAGES_FOLDER/$LINUX_IMAGE

        # Target linux root folder
        ROOT_PATH=$TEMP_DIR/$LINUX_HOME_ROOT
        APP_PATH=$ROOT_PATH/$1/$CURRENT_APPLICATION

        # Unpack the linux image
        "$LINUX_IMAGES_FOLDER/utilities/unpackImage.sh" "$SOURCE_IMG" "$TEMP_DIR" 2> /dev/null

        # Copy to the disk image
        mkdir -p "$APP_PATH"
        if [[ "$APPLICATIONS_NAME" ]]; then
                cp -rf "$APPLICATIONS_NAME" "$APP_PATH"
        fi
        cp -rf "$LINUX_RUNSCRIPT" "$ROOT_PATH"
        cp -rf "$OVPFIM_EXEC" "$ROOT_PATH"

        if [[ "$CURRENT_APPLICATION" == "darknet-yolov2" ]]; then
                cp -rf cfg data "$APP_PATH"
        else
                DATA_FOLDER=$APPLICATIONS_FOLDER/$CURRENT_APPLICATION/Data
                if [[ -d "$DATA_FOLDER" ]]; then
                        cp -rf "$DATA_FOLDER" "$APP_PATH"
                fi
        fi

        # Change permissions
        chmod 777 -R "$ROOT_PATH"/*

        # Unpack and clean
        "$LINUX_IMAGES_FOLDER/utilities/packImage.sh" "$TEMP_DIR" "./$LINUX_IMAGE" 2> /dev/null
        rm -rf "$TEMP_DIR"

        # Create the AXF file for the big.LITTLE
        if [[ "$ARCHITECTURE" = "ARM_V8_BL_2_2" ]] || [[ "$ARCHITECTURE" = "ARM_V8_BL_2_4" ]] ; then
                echo "Creating AXF file"
                WRAPPER_LINUX_KERNEL_DIR="linux-linaro-tracking-linux-linaro-4.3-2015.11"
                WRAPPER_DIR="$LINUX_FOLDER/boot-wrapper-aarch64"

                # First create a symbolic link in the kernel folder
                ln -fs "$LINUX_IMAGES_FOLDER/$LINUX_KERNEL" "$LINUX_FOLDER/$WRAPPER_LINUX_KERNEL_DIR/arch/arm64/boot/Image"

                # Create the AXF using the wrapper
                cd "$WRAPPER_DIR" || exit
                make clean > "$WORKING_FOLDER/make.wrapper"
                autoreconf -i

                # Configure
                CONFIGURE="./configure --host=aarch64-linux-gnu --with-kernel-dir=$LINUX_FOLDER/$WRAPPER_LINUX_KERNEL_DIR --with-dtb=$LINUX_IMAGES_FOLDER/$LINUX_DTB --with-initrd=$WORKING_FOLDER/$LINUX_IMAGE"

                # Check if app make was successful, if not exits the script
                if [[ "$($CONFIGURE > "$WORKING_FOLDER/configure.wrapper")" -ne 0 ]]; then
                        cat "$WORKING_FOLDER/configure.wrapper"
                        exit 1
                fi

                # Compile Check if app make was successful, if not exits the script
                if [[ "$(make > "$WORKING_FOLDER/make.wrapper")" -ne 0 ]]; then
                        cat "$WORKING_FOLDER/make.wrapper"
                        exit 1
                fi

                cp -rf "$LINUX_KERNEL_AXF" "$WORKING_FOLDER"
                cd - > /dev/null || exit

                # Copy back the application elf
                cp -rf "$LINUX_KERNEL_AXF" "$APPLICATIONS_FOLDER/$CURRENT_APPLICATION"
        fi
}


# configure the simulation commands and environment
function configureCommands {
        # Define paths
        # Root folder - All paths will be derived from here automatically
        export PROJECT_FOLDER="$PWD"

        # Working space folder
        export WORKSPACE="$PROJECT_FOLDER/workspace"

        # Workloads folder
        export WORKLOADS="$PROJECT_FOLDER/workloads"

        # Support tools and files
        export SUPPORT_FOLDER="$PROJECT_FOLDER/support"

        # OVPSim platform
        export PLATFORM_FOLDER="$PROJECT_FOLDER/platformOP"

        # OVPSim module
        export MODULE_FOLDER="$PLATFORM_FOLDER/module"

        # Intercept Library-watchdog
        export INTERCEPT_FOLDER="$PLATFORM_FOLDER/intercept"

        # Folder containing the support tools
        export TOOLS_FOLDER="$SUPPORT_FOLDER/tools"

        # Python Version
        export PYTHON_V="python3"

        # Fault generator command
        export CMD_FAULT_LIST="$PYTHON_V $TOOLS_FOLDER/faultList.py"

        # Fault harvest command
        export CMD_FAULT_HARVEST="$PYTHON_V $TOOLS_FOLDER/harvest.py"

        #Fault Classification command
        export CMD_FAULT_GRAPHIC="$PYTHON_V $TOOLS_FOLDER/graphics.py"

        # Profile command
        export CMD_PROFILE="$PYTHON_V $TOOLS_FOLDER/timeout.py"

        # Custom Time
        export CTIME="/usr/bin/time -f \"\t%E real,\t%U user,\t%S sys,\t%M mem\""

        # Suffix for temporary files and folders
        export SUFIX_FIM_TEMP_FILES=FIM_log

        # OVPSim Command
        export CMD_OVP="harness.$IMPERAS_ARCH.exe"

        # OVP_FIM Library
        export OVP_FIM="$SUPPORT_FOLDER/fim-api"

        # OVP cpu type
        CPU_TYPE=OVP

        # Dependency
        export DEPENDENCY=

        # Enumerators
        source "$SUPPORT_FOLDER/variables.sh"

        # Create the workspace folder
        mkdir -p "$WORKSPACE"

        if [[ -z "$PARALLEL" ]]; then # PARALLEL NOT defined
                export PARALLEL=$(($(nproc)/2)) # Default, get the number of processing units available
        fi

        # Simulator dependent defines and variables 0.0000001
        # Common attributes
        CMD_OVP="$CMD_OVP --quantum $TIME_SLICE --faulttype $FAULT_TYPE --targetcpucores $NUM_CORES --interceptlib $PLATFORM_FOLDER/intercept/model.so" #
		
		export vendor=arm.ovpworld.org
        # Workload definition
        case "$WORKLOAD_TYPE" in
                WORKLOAD_BAREMETAL)
                        # Baremetal Applications, ignore the multi-core factor
                        case "$ARCHITECTURE" in
                                ARM_CORTEX_A5) CPU_VARIANT=Cortex-A5UP; export armType=arm; export ENVIRONMENT=ovparmv7; export armSemihost=armNewlib;;
                                ARM_CORTEX_A7) CPU_VARIANT=Cortex-A7UP; export armType=arm; export ENVIRONMENT=ovparmv7; export armSemihost=armNewlib;;
                                ARM_CORTEX_A8) CPU_VARIANT=Cortex-A8; export armType=arm; export ENVIRONMENT=ovparmv7; export armSemihost=armNewlib;;
                                ARM_CORTEX_A9) CPU_VARIANT=Cortex-A9UP; export armType=arm; export ENVIRONMENT=ovparmv7; export armSemihost=armNewlib;;
                                ARM_CORTEX_A15) CPU_VARIANT=Cortex-A15UP; export armType=arm; export ENVIRONMENT=ovparmv7; export armSemihost=armNewlib;;
                                ARM_CORTEX_M0) CPU_VARIANT=Cortex-M0; export armType=armm; export ENVIRONMENT=ovparmv7; export armSemihost=armNewlib;;
                                ARM_CORTEX_M0PLUS) CPU_VARIANT=Cortex-M0plus; export armType=armm; export ENVIRONMENT=ovparmv7; export armSemihost=armNewlib;;
                                ARM_CORTEX_M3) CPU_VARIANT=Cortex-M3; export armType=armm; export ENVIRONMENT=ovparmv7; export armSemihost=armNewlib;;
                                ARM_CORTEX_M4) CPU_VARIANT=Cortex-M4; export armType=armm; export ENVIRONMENT=ovparmv7; export armSemihost=armNewlib;;
                                ARM_CORTEX_M4F) CPU_VARIANT=Cortex-M4F; export armType=armm; export ENVIRONMENT=ovparmv7; export armSemihost=armNewlib; CMD_OVP="$CMD_OVP --override enableVFPAtReset=on";;
                                ARM_CORTEX_M7) CPU_VARIANT=Cortex-M7; export armType=armm; export ENVIRONMENT=ovparmv7; export armSemihost=armNewlib;;
                                ARM_CORTEX_M7F) CPU_VARIANT=Cortex-M7F; export armType=armm; export ENVIRONMENT=ovparmv7; export armSemihost=armNewlib; CMD_OVP="$CMD_OVP --override enableVFPAtReset=on";;
                                ARM_CORTEX_M33) CPU_VARIANT=Cortex-M33; export armType=armm; export ENVIRONMENT=ovparmv7; export armSemihost=armNewlib;;
                                ARM_CORTEX_M33F) CPU_VARIANT=Cortex-M33F; export armType=armm; export ENVIRONMENT=ovparmv7; export armSemihost=armNewlib;;
                                ARM_CORTEX_A53) CPU_VARIANT=Cortex-A53MPx1; export armType=arm; export ENVIRONMENT=ovparmv8; export armSemihost=armAngel;;
                                ARM_CORTEX_A57) CPU_VARIANT=Cortex-A57MPx1; export armType=arm; export ENVIRONMENT=ovparmv8; export armSemihost=armAngel;;
                                ARM_CORTEX_A72) CPU_VARIANT=Cortex-A72MPx1; export armType=arm; export ENVIRONMENT=ovparmv8; export armSemihost=armAngel;;
                                RISCV32GC) CPU_VARIANT=RV32GC; export ENVIRONMENT=riscv; export armType=riscv; export armSemihost=pk; export vendor=riscv.ovpworld.org;;
                                RISCV32I) CPU_VARIANT=RV32I; export ENVIRONMENT=riscv; export armType=riscv; export armSemihost=pk; export vendor=riscv.ovpworld.org;;
                                RISCV32IM) CPU_VARIANT=RV32IM; export ENVIRONMENT=riscv; export armType=riscv; export armSemihost=pk; export vendor=riscv.ovpworld.org;;
                                RISCV32IMAC) CPU_VARIANT=RV32IMAC; export ENVIRONMENT=riscv; export armType=riscv; export armSemihost=pk; export vendor=riscv.ovpworld.org;;
                                RISCV64IMAC) CPU_VARIANT=RV64IMAC; export ENVIRONMENT=riscv; export armType=riscv; export armSemihost=pk; export vendor=riscv.ovpworld.org;;
                                RISCV64GC) CPU_VARIANT=RV64GC; export ENVIRONMENT=riscv; export armType=riscv; export armSemihost=pk; export vendor=riscv.ovpworld.org;;
                                *) echo "Invalid architecture"; exit ;;
                        esac

                        # Bare-metal support folder
                        export BAREMETAL_FOLDER=$SUPPORT_FOLDER/baremetal

                        # Copy the tcl module bare metal
                        cp -rf "$MODULE_FOLDER/module.op.tcl.baremetal" "$MODULE_FOLDER/module.op.tcl"

                        # Applications source
                        APPLICATIONS_FOLDER=$WORKLOADS/baremetal

                        # Simulator caller
                        CMD_OVP="$CMD_OVP --mode baremetal --variant $CPU_VARIANT --verbose -environment $ENVIRONMENT --arch singlecore "

                        # Flags to export to the Makefile
                        export MAKEFILE_CFLAGS="-g -w -I$SUPPORT_FOLDER -I$OVP_FIM -D$ENVIRONMENT -DBAREMETAL -std=gnu99 -O$OPTFLAG"

                        # Overrides specific by architecture
                        if [[ "$ARCHITECTURE" = 'ARM_CORTEX_M0' ]] || \
							[[ "$ARCHITECTURE" = 'ARM_CORTEX_M0PLUS' ]] || \
							[[ "$ARCHITECTURE" = 'ARM_CORTEX_M3' ]] || \
							[[ "$ARCHITECTURE" = 'ARM_CORTE_M4' ]] || \
							[[ "$ARCHITECTURE" = 'ARM_CORTEX_M4F' ]] || \
							[[ "$ARCHITECTURE" = 'ARM_CORTEX_M7' ]] || \
							[[ "$ARCHITECTURE" = 'ARM_CORTEX_M7F' ]] || \
							[[ "$ARCHITECTURE" = 'ARM_CORTEX_M33' ]] || \
							[[ "$ARCHITECTURE" = 'ARM_CORTEX_M33F' ]]  ; then
                                CMD_OVP="$CMD_OVP --override compatibility=nopBKPT "
                        fi
                        #~ elif [[ "$ARCHITECTURE" = 'ARM_CORTEX_A53' ]] || [[ "$ARCHITECTURE" = 'ARM_CORTEX_A57' ]] || [[ "$ARCHITECTURE" = 'ARM_CORTEX_A72' ]]; then
                                #~ CMD_OVP="$CMD_OVP"
                        #~ elif [[ "$ARCHITECTURE" = 'ARM_CORTEX_M0' ]]; then
                                #~ CMD_OVP="$CMD_OVP --override compatibility=nopSVC "
                        #~ fi
                        
                        ;; # End baremetal
                WORKLOAD_LINUX)
                        case "$ARCHITECTURE" in
                                ARM_CORTEX_A9)
                                        # Environment
                                        export ENVIRONMENT=ovparmv7

                                        # MPI library
                                        export MPI_LIB=$SUPPORT_FOLDER/mpich-3.2-armv7 # Mpi library path
                                        export MPICALLER=armv7-linux-gnueabi-mpirun # Mpi app caller

                                        export LINUX_FOLDER=$SUPPORT_FOLDER/linux-armv7 # Linux support folder
                                        export LINUX_IMAGES_FOLDER=$LINUX_FOLDER/disks # Folder with the full-system images
                                        export LINUX_IMAGE_ORIGINAL=fs.img # Linux image
                                        export LINUX_IMAGE=$LINUX_IMAGE_ORIGINAL.$WORKLOAD_TYPE.$ENVIRONMENT.$NUM_CORES # Loaded image - temporary -> no git track
                                        export LINUX_KERNEL=zImage # Kernel binary
                                        export LINUX_VKERNEL=vmlinux # Kernel binary
                                        export LINUX_BOOTLOADER=smpboot.ARM_CORTEX_A9.elf # Bootloader binary
                                        export LINUX_HOME_ROOT=root # Target linux root folder
                                        export LINUX_RUNSCRIPT=run.sh # Script responsible to call the application inside the linux
                                        export OVPFIM_EXEC=$OVP_FIM/OVPFIM-ARMv7.elf # Runtime fim caller

                                        # Copy the tcl module
                                        cp -rf "$MODULE_FOLDER/module.op.tcl.linux" "$MODULE_FOLDER/module.op.tcl"

                                        # CrossCompiler Linux
                                        #~ export ARM_TOOLCHAIN_LINUX=/usr/bin
#~ #                                        export ARM_TOOLCHAIN_LINUX=/$HOME/Downloads/gcc-linaro-5.4.1-2017.05-x86_64_arm-linux-gnueabi
                                        #~ export ARM_TOOLCHAIN_LINUX=/usr
                                        #~ export ARM_TOOLCHAIN_LINUX_PREFIX=arm-linux-gnueabi

                                        CMD_OVP="$CMD_OVP --mode linux --startaddress 0x60000000 --arch multicore"

                                        case "$ARCHITECTURE" in
                                                ARM_CORTEX_A9) CPU_VARIANT=Cortex-A9MPx$NUM_CORES; MPUFLAG="cortex-a9";;
                                                ARM_CORTEX_A7) CPU_VARIANT=Cortex-A7MPx$NUM_CORES; MPUFLAG="cortex-a7";;
                                        esac

                                        # Define the workload path
                                        case "$WORKLOAD_TYPE" in
                                                WORKLOAD_LINUX) APPLICATIONS_FOLDER=$WORKLOADS/linux;;
                                                *) echo "Invalid Workload!"; exit ;;
                                        esac

                                        # Check if the boot loader is compiled
                                        if [[ ! -e "$LINUX_IMAGES_FOLDER/$LINUX_BOOTLOADER" ]]; then
                                                echo "File $LINUX_BOOTLOADER do not found; Compiling"
                                                make -C "$LINUX_IMAGES_FOLDER" VERBOSE=1
                                        fi

                                        CMD_OVP="$CMD_OVP --objfilenoentry $LINUX_BOOTLOADER"

                                        ;; # End ARM_CORTEX_A9
                                ARM_CORTEX_A53 | ARM_CORTEX_A57 | ARM_CORTEX_A72)
                                        # Environment
                                        export ENVIRONMENT=ovparmv8

                                        # MPI library
                                        export MPI_LIB=$SUPPORT_FOLDER/mpich-3.2-armv8 # Mpi library path
                                        export MPICALLER=armv8-linux-gnueabi-mpirun # Mpi app caller

                                        # Copy the tcl module
                                        cp -rf "$MODULE_FOLDER/module.op.tcl.linux64" "$MODULE_FOLDER/module.op.tcl"

                                        export LINUX_FOLDER=$SUPPORT_FOLDER/linux-armv8 # Linux support folder
                                        export LINUX_IMAGES_FOLDER=$LINUX_FOLDER/disks # Folder with the full-system images
                                        export LINUX_IMAGE_ORIGINAL=initrd.arm64.img # Kernel binary
                                        export LINUX_IMAGE=$LINUX_IMAGE_ORIGINAL.$WORKLOAD_TYPE.$ENVIRONMENT.$NUM_CORES # Loaded image - temporary -> no git track
                                        export LINUX_KERNEL=Image.defconfig # Linux image
                                        export LINUX_VKERNEL=vmlinux.defconfig # Linux kernel
                                        export LINUX_BOOTLOADER="" # Linux boatloader binary
                                        export LINUX_HOME_ROOT=root # Target linux root folder
                                        export LINUX_RUNSCRIPT=run.sh # Script responsible to call the application inside the linux
                                        export OVPFIM_EXEC=$OVP_FIM/OVPFIM-ARMv8.elf # Runtime fim caller

                                        # CrossCompiler Linux AARCH64
                                        #~ export ARM_TOOLCHAIN_LINUX=/usr
                                        #~ export ARM_TOOLCHAIN_LINUX_PREFIX=aarch64-linux-gnu

                                        # The arm A72 requires a different dtb
                                        if [[ "$ARCHITECTURE" = "ARM_CORTEX_A72" ]]; then
                                                export LINUX_DTB=foundation-v8-gicv3.dtb # Dtb files
                                        else
                                                export LINUX_DTB=foundation-v8.dtb # Dtb files
                                        fi

                                        CMD_OVP="$CMD_OVP --mode linux64 --linuxdtb $LINUX_DTB --startaddress 0x80000000 --arch multicore"

                                        case "$ARCHITECTURE" in
                                                ARM_CORTEX_A53) CPU_VARIANT=Cortex-A53MPx$NUM_CORES; MPUFLAG="cortex-a53";;
                                                ARM_CORTEX_A57) CPU_VARIANT=Cortex-A57MPx$NUM_CORES; MPUFLAG="cortex-a57";;
                                                ARM_CORTEX_A72) CPU_VARIANT=Cortex-A72MPx$NUM_CORES; MPUFLAG="cortex-a72";;
                                        esac

                                        # Define the workload path
                                        case "$WORKLOAD_TYPE" in
                                                WORKLOAD_LINUX) APPLICATIONS_FOLDER=$WORKLOADS/linux;;
                                                *) echo "Invalid Workload!"; exit ;;
                                        esac

                                        ;; # End ARMv8
                                ARM_V8_BL_2_2 | ARM_V8_BL_2_4)
                                        # Environment
                                        export ENVIRONMENT=ovparmv8

                                        # MPI library
                                        export MPI_LIB=$SUPPORT_FOLDER/mpich-3.2-armv8 # Mpi library path
                                        export MPICALLER=armv8-linux-gnueabi-mpirun # Mpi app caller

                                        # Copy the tcl module
                                        cp -rf "$MODULE_FOLDER/module.op.tcl.linux64" "$MODULE_FOLDER/module.op.tcl"

                                        export LINUX_FOLDER=$SUPPORT_FOLDER/linux-armv8 # Linux support folder
                                        export LINUX_IMAGES_FOLDER=$LINUX_FOLDER/disks # Folder with the full-system images
                                        export LINUX_IMAGE_ORIGINAL=initrd.arm64.img # Kernel binary
                                        export LINUX_IMAGE=$LINUX_IMAGE_ORIGINAL.$WORKLOAD_TYPE.$ENVIRONMENT.$NUM_CORES # Loaded image - temporary -> no git track
                                        export LINUX_KERNEL=Image.defconfig # Linux image
                                        export LINUX_VKERNEL=vmlinux.defconfig # Linux kernel
                                        export LINUX_KERNEL_AXF=linux-system.axf # Linux kernel AXF format
                                        export LINUX_BOOTLOADER="" # Linux boatloader binary
                                        export LINUX_HOME_ROOT=root # Target linux root folder
                                        export LINUX_RUNSCRIPT=run.sh # Script responsible to call the application inside the linux
                                        export OVPFIM_EXEC=$OVP_FIM/OVPFIM-ARMv8.elf # Runtime fim caller

                                        # CrossCompiler Linux AARCH64
                                        export ARM_TOOLCHAIN_LINUX=/usr
                                        export ARM_TOOLCHAIN_LINUX_PREFIX=aarch64-linux-gnu

                                        # The arm BL requires a different dtb
                                        export LINUX_DTB=multicluster-57x2_53x4-GICv2.dtb

                                        CMD_OVP="$CMD_OVP --mode linux64 --program $LINUX_KERNEL_AXF --arch bigLITTLE"

                                        case "$ARCHITECTURE" in
                                                ARM_V8_BL_2_2) CPU_VARIANT=MultiCluster; NUM_CORES=4 ;MPUFLAG="cortex-a57.cortex-a53"; CMD_OVP+=" --override FIM/DUT0/cpu/override_clusterVariants=Cortex-A57MPx2,Cortex-A53MPx2";;
                                                ARM_V8_BL_2_4) CPU_VARIANT=MultiCluster; NUM_CORES=4 ;MPUFLAG="cortex-a57.cortex-a53"; CMD_OVP+=" --override FIM/DUT0/cpu/override_clusterVariants=Cortex-A57MPx2,Cortex-A53MPx4";;
                                        esac

                                        # Define the workload path
                                        case "$WORKLOAD_TYPE" in
                                                WORKLOAD_LINUX) APPLICATIONS_FOLDER=$WORKLOADS/linux;;
                                                *) echo "Invalid Workload!"; exit ;;
                                        esac

                                        ;; # End ARMv8
                                *) echo "Invalid Workload processor match"; exit ;;
                        esac

                        # Flags to export to the Makefile
                        export NUM_THREADS="$NUM_CORES"
                        export MAKEFILE_CFLAGS="-O3 -g -w -gdwarf-2 -mcpu=$MPUFLAG -mlittle-endian -DUNIX -static -I$OVP_FIM -D$ENVIRONMENT -DOPEN -DNUM_THREAD=4 -fopenmp -pthread"
                        export MAKEFILE_FLINKFLAGS="-static -fopenmp -I$OVP_FIM -D$ENVIRONMENT -lm -lstdc++"

                        # Check if the OVPFIM run time caller is compiled
                        if [[ ! -e "$OVPFIM_EXEC" ]]; then
                                echo "File $OVPFIM_EXEC do not found; Compiling ..."
                                make "$ENVIRONMENT" -C "$OVP_FIM" > "$OVP_FIM/make.ovpfim"
                        fi

                        MTUNE=$(echo "$CPU_VARIANT" | cut -d 'M' -f 1)
                        export MTUNE

                        # Common linux arguments --objfilenoentry $LINUX_BOOTLOADER --boot $LINUX_BOOTLOADER
                        CMD_OVP="$CMD_OVP --verbose --variant $CPU_VARIANT --symbolfile $LINUX_VKERNEL --symbolfile $OVPFIM_EXEC --zimage $LINUX_KERNEL --initrd $LINUX_IMAGE -environment $ENVIRONMENT"

                        # Enable uart console window
                        if [[ $ENABLE_CONSOLE -eq 1 ]]; then
                                CMD_OVP="$CMD_OVP --enableconsole"
                        fi

                        # Enable graphical interface
                        if [[ $ENABLE_GRAPHIC -eq 1 ]]; then
                                CMD_OVP="$CMD_OVP --linuxgraphics"
                        fi
                        ;; # End Linux
                *) echo "Invalid Workload!"; exit ;;
        esac

        ### SYNC POINT ####
        # Count the number of visited folders
        COUNTER=1

        # Enter in the folder containing the applications
        cd "$APPLICATIONS_FOLDER" || exit

        # Get the folders in the target application root folder
        if [[ -z "$APPLICATION_LIST" ]]; then
                APPLICATION_LIST=($(ls))
        fi

        VISITED_FOLDERS="${#APPLICATION_LIST[@]}"

        # Test if the ENVIRONMENT was set
        if [[ -z "$ENVIRONMENT" ]]; then
                echo "ENVIRONMENT not set!" && exit
        fi
}



################################################################################################
# entry time stamp
entertime="$(date +%s%N)"
# clean the terminal
#reset
############################### simulation variables ###########################################

source config

# configure the simulator command according with the options
configureCommands

################################################################################################
############################### Fault Campaign Began ###########################################
################################################################################################
echo "**************************************************************"
echo -e "\e[31mCompiling the Platform and Intercept Library \e[0m"
echo "**************************************************************"
OPTF=1
# module
if [[ "$(make clean all VERBOSE=1 OPT=$OPTF -C "$MODULE_FOLDER" &> "$WORKSPACE/make.module")" -ne 0 ]]; then
		cat "$WORKSPACE/make.module"
		exit 1
fi # Check if make was successfully, if not exits the script

# intercept library
if [[ "$(make clean all VERBOSE=1 OPT=$OPTF -C "$INTERCEPT_FOLDER" &> "$WORKSPACE/make.intercept")" -ne 0 ]]; then
		cat "$WORKSPACE/make.intercept"
		exit 1
fi # Check if make was successful, if not exits the script

# accessing each application folder
for applicationFolders in ${APPLICATION_LIST[*]}
do
        echo "**************************************************************"
        echo -e "\e[31mCompiling the Harness \e[0m"
        echo "**************************************************************"
        # Harness (GOLD)
        export NODES=1;

		if [[ "$(make clean all VERBOSE=1 OPT=$OPTF -C "$PLATFORM_FOLDER/harness" &> "$WORKSPACE/make.harness")" -ne 0 ]]; then
				cat "$WORKSPACE/make.harness"
				exit 1
		fi # Check if make was successfully, if not exits the script
		
        # Remove the last slash
        CURRENT_APPLICATION=${applicationFolders%*/}

        echo "**************************************************************"
        echo -e "\e[31mApplication beginning:\e[0m $CURRENT_APPLICATION"
        echo "**************************************************************"
        echo "$CURRENT_APPLICATION" "$(date +"%T\ %D")" >> "$WORKSPACE/applications"

        time="$(($(date +%s%N)-entertime))"
        echo "$CURRENT_APPLICATION" "$((time/1000000000))" >> "$WORKSPACE/timing"

        # Clean the folder and compile the application
        compileApplication

        # Next application if LI_JUST_COMPILE
        if [[ $LI_OPTIONS -eq $LI_JUST_COMPILE ]]; then
                echo "LI_JUST_COMPILE"
                folderCounter
                continue
        fi

        echo "**************************************************************"
        echo -e "\e[31mBeginning Gold Execution\e[0m"
        echo "**************************************************************"
        # sudo tunctl -u frdarosa -t tap0
        # sudo ifconfig tap0 192.168.9.4 up
        # SIM_ARGS+=" --tracesymbol p --override FIM/DUT0/eth0/tapDevice=tap0 "
        if [[ -n "$SYMBOL" ]]; then
                SIM_ARGS+=" --tracesymbol $SYMBOL"
        fi

        # Setup gold command
        GOLD_RUN="./$CMD_OVP $SIM_ARGS --platformid 0 --numberoffaults 1 --goldrun --applicationfolder $WORKING_FOLDER"

        # Get time before running gold
        GOLD_EXEC=$SECONDS
        
        if [[ $ENABLE_TRACE -eq 1 ]]; then
			GOLD_RUN="$GOLD_RUN --trace "
		fi
                        
        if [[ $ENABLE_PROFILING -eq 1 ]]; then
			GOLD_RUN="$GOLD_RUN --enabletools --extlib FIM/DUT0/cpu/cpuEstimator=${PROJECT_FOLDER}/cpuEstimator/model.so --callcommand \"FIM/DUT0/cpu_CPU0/cpuEstimator/instTrace -on\" --enableftrace"
        fi

        # Run the gold platform
        # Check if the gold run was successful, if not exits the script
		echo "$GOLD_RUN"
		$GOLD_RUN |& tee ./PlatformLogs/goldlog || exit
		
		if [[ "$ONLY_GOLD" -eq 1 ]]; then
			# Exit after N folders
			# Return to the applications folder
			cd "$APPLICATIONS_FOLDER" || exit
			if [[ "$COUNTER" -eq "$VISITED_FOLDERS" ]]; then
					break
			else
					COUNTER=$((COUNTER+1))
					continue
			fi
		fi
		
        # Get run time of gold execution
        GOLD_EXEC=$((SECONDS - GOLD_EXEC))

        GOLD_EXEC=$((GOLD_EXEC*3))

        # Create the checkpoints when required
        generateCheckpoints

        # Generating a fault list
        generateFaultList2

        # Harness (FI)
        export NODES=$FAULTS_PER_PLATFORM;

		make clean all VERBOSE=1 OPT=$OPTF -C "$PLATFORM_FOLDER/harness" &> "$WORKSPACE/makeh2"
		cp -rf "$PLATFORM_FOLDER/harness"/*exe "$WORKING_FOLDER"
		
        echo "**************************************************************"
        echo -e "\e[31mBeginning Fault Injection Campaign \e[0m"
        echo "**************************************************************"

        time="$(($(date +%s%N)-entertime))"
        echo "Fault_Injection $((time/1000000000))" >> "$WORKSPACE/timing"

        SIM_ARGS="$SIM_ARGS --faultcampaignrun --checkpointinterval $CHECKPOINT_INTERVAL --numberoffaults $FAULTS_PER_PLATFORM"

        echo "${GOLD_EXEC}" > timeOutTime

        SIM_RUN="./$CMD_OVP $SIM_ARGS --applicationfolder $WORKING_FOLDER --platformid"

        # Time stamp for the fault injection
        sFaultCampaignBegin="$(date +%s%N)"
        export sFaultCampaignBegin

		# Create script to init ovp
		python2 "${PROJECT_FOLDER}/initOvp.py" -l "${LICENSE}" -v "${IMPERAS_VERSION}"
		# Execute the FI platform
		python2 "${PROJECT_FOLDER}/submitJobs.py" -g "${GOLD_RUN}" -r "${SIM_RUN}" -w "${PARALLEL}" -f "${NUMBER_OF_FAULTS}" -a "${CURRENT_APPLICATION}" -c "${CLUSTER}"
		# Execute fault harvest
		callHarvestLocal
		# Ending
		#cat ./*reportfile
		cp -rf ./*reportfile "$WORKSPACE"

        # Exit after N folders
        # Return to the applications folder
        cd "$APPLICATIONS_FOLDER" || exit
        if [[ "$COUNTER" -eq "$VISITED_FOLDERS" ]]; then
                break
        else
                COUNTER=$((COUNTER+1))
        fi

done # For dir

# Back to the root folder
cd .. || exit

echo "**************************************************************"
echo "Simulation end"
echo "**************************************************************"
time="$(($(date +%s%N)-entertime))"
echo "Simulation_End" "$((time/1000000000))" >> "$WORKSPACE/timing"

