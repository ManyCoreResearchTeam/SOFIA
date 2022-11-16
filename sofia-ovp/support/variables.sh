#!/bin/bash
#~ set +euo

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


enum index { WORKLOAD_BAREMETAL, WORKLOAD_FREERTOS, WORKLOAD_LINUX, WORKLOAD_RODINIA, WORKLOAD_NPB_OMP, WORKLOAD_NPB_SER, WORKLOAD_NPB_MPI, WORKLOAD_CASE_STUDY, WORKLOAD_CASE_STUDY_DARK };
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
            sleep $((RANDOM % 20 + 5))
        done
    }

#
# Functions to change the script variables
#
function configureExecutions            {
    find . -name 'FI.sh' | xargs sed -i 's/NUMBER_OF_FAULTS=[0-9]\{1,6\}/NUMBER_OF_FAULTS='$1'/g'
    }
function configureMode                  {
    find . -name 'FI.sh' | xargs sed -i 's/MODE=[0-9A-Za-z_]*/MODE='$1'/g'
    }
function configureCPUType               {
    find . -name 'FI.sh' | xargs sed -i 's/CPU_TYPE=[0-9A-Za-z_]*/CPU_TYPE='$1'/g'
    }
function configureHostCores             {
    find . -name 'FI.sh' | xargs sed -i 's/PARALLEL=[0-9]\{1,2\}/PARALLEL='$1'/g'
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
function configureTimeslice             {
    find . -name 'FI.sh' | xargs sed -i 's/TIME_SLICE=[0-9A-Za-z_.]*/TIME_SLICE='$1'/g'
    }
function configureWorkload              {
    find . -name 'FI.sh' | xargs sed -i 's/WORKLOAD_TYPE=[0-9A-Za-z_.]*/WORKLOAD_TYPE='$1'/g'
    }

#set -euo
