#!/bin/bash

# This funcion allow to declare enum "types", I guess
enum ()
{
    # skip index ???
    shift
    AA=${@##*\{} # get string strip after {
    AA=${AA%\}*} # get string strip before }
    AA=${AA//,/} # delete commaa
    ((DEBUG)) && echo $AA
    local I=0
    for A in $AA ; do
        eval "$A=$I"
        ((I++))
    done
}

enum index { FIM_SE_MODE, FIM_FS_MODE, FIM_BA_MODE, ORIGINAL_SE_MODE, ORIGINAL_FS_MODE, ORIGINAL_BA_MODE };
enum index { ATOMIC, TIMING, DETAILED };
enum index { WORKLOAD_BAREMETAL, WORKLOAD_RODINIA, WORKLOAD_PTHREAD, WORKLOAD_NPB_OMP, WORKLOAD_NPB_SER, WORKLOAD_NPB_MPI, WORKLOAD_MIBENCH , WORKLOAD_WCET , WORKLOAD_CASE_STUDY};
enum index { NO_GENERATION, GENERATE_FAULTS, NO_FAULTS };
enum index { LI_USE_CLEAN, LI_APPEND, LI_DONT_COMPILE , LI_JUST_COMPILE };

#
# Queue functions
#
function queue {
    QUEUE="$QUEUE $1"
    NUM=$(($NUM+1))
}

function regeneratequeue {
    OLDREQUEUE=$QUEUE
    QUEUE=""
    NUM=0
    for PID in $OLDREQUEUE
    do
        if [ -d /proc/$PID  ] ; then
            QUEUE="$QUEUE $PID"
            NUM=$(($NUM+1))
        fi
    done
}

function checkqueue {
    OLDCHQUEUE=$QUEUE
    for PID in $OLDCHQUEUE
    do
        if [ ! -d /proc/$PID ] ; then
            regeneratequeue # at least one PID has finished
            break
        fi
    done
}

# exit after N cores
function folderCounter {
    #Return to the applications folder
    cd $APPLICATIONS_FOLDER
    if [[ "$COUNTER" -eq "$VISITED_FOLDERS" ]]; then
        break
    else
        COUNTER=$((COUNTER+1))
    fi
}

# limit the number of running cores
function parallelExecutionManager {
        #Aquire the last PID and insert in the queue
        PID=$!
        queue $PID

        #Check to number of running platforms
        while [ $NUM -ge $MAX_NPROC ]; do
            checkqueue
            sleep 0.1
        done
    }

#
# Functions to change the script variables
#
function configureExecutions            {
    find . -name 'FI.sh' | xargs sed -i 's/PLATFORM_EXECUTIONS=[0-9]\{1,6\}/PLATFORM_EXECUTIONS='$1'/g'
    }
function configureMode                  {
    find . -name 'FI.sh' | xargs sed -i 's/MODE=[0-9A-Za-z_]*/MODE='$1'/g'
    }
function configureCPUType               {
    find . -name 'FI.sh' | xargs sed -i 's/CPU_TYPE=[0-9A-Za-z_]*/CPU_TYPE='$1'/g'
    }
function configureHostCores             {
    find . -name 'FI.sh' | xargs sed -i 's/MAX_NPROC=[0-9]\{1,2\}/MAX_NPROC='$1'/g'
    }
function configureTargetCores           {
    find . -name 'FI.sh' | xargs sed -i 's/NUM_CORES=[0-9]\{1,2\}/NUM_CORES='$1'/g'
    }
function configureFaultList             {
    find . -name 'FI.sh' | xargs sed -i 's/GENERATE_FAULT_LIST=[0-9A-Za-z_]*/GENERATE_FAULT_LIST='$1'/g'
    }
function configureNumberCheckpoint      {
    find . -name 'FI.sh' | xargs sed -i 's/NUMBER_CHKP=[0-9]/NUMBER_CHKP='$1'/g'
    }
function configureCheckpointInterval    {
    find . -name 'FI.sh' | xargs sed -i 's/CHECKPOINT_INTERVAL=[0-9]\{1,10\}/CHECKPOINT_INTERVAL='$1'/g'
    }
function configureVisitedFolders        {
    find . -name 'FI.sh' | xargs sed -i 's/VISITED_FOLDERS=[0-9]\{1,10\}/VISITED_FOLDERS='$1'/g'
    }
