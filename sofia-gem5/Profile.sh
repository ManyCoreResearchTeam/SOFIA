#!/bin/bash
    #Root folder
    PROJECT_FOLDER=$(pwd)
    
    #Folder containing tools for the Fault Injector
    TOOLS_FOLDER=$PROJECT_FOLDER/Support/Tools
    
    #Fault harvest command
    CMD_FAUL_HARVEST=$TOOLS_FOLDER/FaultHarvest.py
    
    #Profile command
    CMD_PROFILE="$TOOLS_FOLDER/timeout.py -pf"
    
    #Clean
    rm -rf *.log
    
    #Call the command using the maximum memory RAM
    $CMD_PROFILE -m  $(echo $(cat /proc/meminfo | grep MemTotal | cut -d':' -f2 | cut -c -16)) ./FI.sh
    
    #Create the graphical representation
    $CMD_FAUL_HARVEST --FIM_graphic_Memory_Trace --FIM_GEM5_ARM
