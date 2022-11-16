#!/bin/bash

#Number of CPU's
cores=$(($(nproc)/2)) #default, get the number of processing units available

results="./Results"

#~ rm -rf $results
mkdir $results

source ./support/variables.sh
#~ VP="OVP"
VP="GEM5"

configureExecutions             6
configureHostCores              1
configureTargetCores            4
configureFaultList              "NO_FAULTS"
configureNumberCheckpoint       0
configureCheckpointInterval     0
configureVisitedFolders         10
configureWorkload               "WORKLOAD_NPB_MPI"

if [ $VP=="OVP" ]; then
    configureTimeslice          0.001
else
    configureMode               "FIM_FS_MODE"
    configureCPUType            "DETAILED"
fi

####################################################################################################
#                                  Rodinia                                                         #
####################################################################################################
    for i in 1 2 3 4
    do
        echo "##### Core $i ##### "
        configureHostCores      $i
        configureExecutions     $i
        
        #~ ./Profile.sh
        ./FI.sh
        
        resultsFolder="$results/mpi-$i"
        
        mkdir $resultsFolder
        
        cp workspace/*report* $resultsFolder
    done

