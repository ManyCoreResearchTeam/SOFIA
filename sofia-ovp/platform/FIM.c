#include "FIM.h"

/*
 * Fault Injection Version - 3.01
 */

/**                                     Global Variables                                  **/
//Main struct pointer
struct processorStruct* vectorProcessor;

// Defines the number of CPUs that will use the Watchdog
int NumberOfPossibleRegisters;
bool removeDumpFiles = true;

//Options
optparse::Values options;

extern char modelVariant[STRING_SIZE];
int systemReturn=0;
char* fgetsReturn;
/**                                     Functions - Report                                **/
/** ------------------------------------------------------------------------------------- **/
void report(){
char temp[256];
if(IsGoldExecution) {
    ///Only one gold count
    int processorNumber = 0;
    char GoldName[STRING_SIZE];

    ///Save the number of executed instruction (main object)
    Uns64 goldCount=0;
    ///Acquire the gold count and the actual instruction count
    for (int i=0;i<NumberOfRunningCores;i++) {
        goldCount+=PE.ApplicationEnd[i];
    }
    ///Save the final PE memory state
    sprintf(GoldName,"%s/%s-%d",dumpDirectory,goldMemoryDumpFilesPrefix,0);
    //~ icmFlushMemory(PE.memObj,BASE_MEMORY,SIZE_MEMORY);
    icmFlushMemory(PE.memObj,0x0,0xFFFFFFFFF);///Flush the memory to avoid OVP dump format glitch
    icmMemorySaveStateFile(PE.memObj,GoldName);

    ///Save the final PE internal state
    icmClearICountBreakpoint(PE.processorObj);
    sprintf(GoldName,"%s/%s-%d",dumpDirectory,goldRegisterDumpFilesPrefix,0);
    icmProcessorSaveStateFile(PE.processorObj,GoldName);

    switch(NumberOfRunningCores) {
        case 1:
            sprintf(temp," sed -i \'1,17d\' %s",GoldName); break;
        case 2:
            sprintf(temp," sed -i \'1,28d\' %s",GoldName); break;
        case 4:
            sprintf(temp," sed -i \'1,50d\' %s",GoldName); break;
    }
    systemReturn=system(temp);

    FILE * pFile = fopen ("Information.FIM_log","w");

    fprintf(pFile,"Application # of instructions="FMT_64u"\n",goldCount);

    if(IsBareMetalMode) {
        fprintf(pFile,"Executed Instructions Per core="FMT_64u"\n",PE.ApplicationEnd[0]);
        fprintf(pFile,"Application begins="FMT_64u"\n",PE.ApplicationBegan[0]);
    } else {
        ///
        fprintf(pFile,"Executed Instructions Per core="FMT_64u"",PE.ApplicationEnd[0]);
        for (int i=1;i<NumberOfRunningCores;i++)
            fprintf(pFile,","FMT_64u"",PE.ApplicationEnd[i]);
        fprintf(pFile,"\n");
        ///
        fprintf(pFile,"Application begins="FMT_64u"",PE.ApplicationBegan[0]);
        for (int i=1;i<NumberOfRunningCores;i++)
            fprintf(pFile,","FMT_64u"",PE.ApplicationBegan[i]);
        fprintf(pFile,"\n");
    }

    fprintf(pFile,"Kernel Boot # of instructions=0\n");
    fprintf(pFile,"Number of Ticks=0\n");

    fclose (pFile);

} else {
    char self_faultIndex                [STRING_SIZE];
    char self_faultType                 [STRING_SIZE];
    char self_faultRegister             [STRING_SIZE];
    char self_faultRegisterIndex        [STRING_SIZE];
    char self_faultTime                 [STRING_SIZE];
    char self_faultMask                 [STRING_SIZE];
    char self_faultAnalysis_name        [STRING_SIZE];
    char self_faultAnalysis_value       [STRING_SIZE];
    char self_ExecutedTicks             [STRING_SIZE];
    char self_ExecutedInstructions      [STRING_SIZE];
    char Self_MemInconsistency          [STRING_SIZE];
    char self_correctCheckpoint         [STRING_SIZE];

    int code=0,memCmp=2,regCmp=2,processorNumber;
    Uns64 Count,GoldCount;
    Uns64 TotalExecInst=0;
    Uns64 CheckpointLoaded = 0;

    char DumpName[STRING_SIZE];
    char GoldName[STRING_SIZE];
    char reportFile[STRING_SIZE];
    char cmpReturn[256];
    FILE * pFileGold;

 /********************************************/
    sprintf(reportFile,"./Reports/report_platform_%d.FIM_log",PlatformIdentifier);
    FILE * pFile = fopen (reportFile,"w");

    ///create file header
    fprintf(pFile,"%8s %7s %10s %2s %18s %7s %28s %6s %25s %22s %18s %12s\n",
            "Index","Type","Target","i","Insertion Time","Mask","Fault Injection Result",
            "Code", "Execution Time (Ticks)", "Executed Instuctions", "Mem Inconsistency",
            "Checkpoint");

 /********************************************/
    ///PE (fault) processing
    for (processorNumber=0; processorNumber<NumberOfRunningFaults; processorNumber++) {
        strcpy(Self_MemInconsistency,"Null");

        TotalExecInst = 0; //Number of executed instructions in all cores
        Count         = 0;
        GoldCount     = 0;

        ///Acquire the gold count and the actual instruction count
        for (int i=0;i<NumberOfRunningCores;i++) {
            Count       += PE.ApplicationEnd[i];
            GoldCount   += PE.expectedICount[i];
        }

        ///Number of simulated instructions
        if(IsCheckpointEnable) { /// !!!! Only valid to one core processor
            TotalExecInst    += Count - ( PE.CHCKpoint * (Uns64) CheckpointInterval);
            CheckpointLoaded  = PE.CHCKpoint*CheckpointInterval;
        } else {
            TotalExecInst+=Count;
        }

        //~ printf("Loaded "FMT_64u" - "FMT_64u" \n",CheckpointLoaded,PE.faultPos);

        ///Get PE stop reason
        int StopReason = icmGetStopReason(PE.processorObj);

        //~ printf("PE.StopReason %u \n",PE.StopReason);
        ///No exception was detected during the simulation
              if (PE.StopReason == FIM_SR_Unset) {
                PE.StopReason=StopReason;
                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                ///Memory comparison
                icmFlushMemory(PE.memObj,0x0,0xFFFFFFFFF);///Flush the memory to avoid OVP dump format glitch

                ///Memory dump files
                sprintf(DumpName,"%s/%s-%d-%d",dumpDirectory,faultMemoryDumpFilesPrefix,processorNumber,PlatformIdentifier);
                sprintf(GoldName,"%s/%s-%d",dumpDirectory,goldMemoryDumpFilesPrefix,0);

                ///Dump File
                icmMemorySaveStateFile(PE.memObj,DumpName);

                ///Compare two text files
                strcpy(cmpReturn,"Null");
                sprintf(temp,"cmp %s %s ",DumpName,GoldName);
                printf("%s\n",temp);
                pFileGold = popen(temp, "r");
                fgetsReturn=fgets(cmpReturn,STRING_SIZE,pFileGold);
                pclose(pFileGold);

                ///Remove Dump file to reduce the HD footprint
                if(removeDumpFiles) {
                    sprintf(temp,"rm %s",DumpName);
                    systemReturn=system(temp);
                }

                //Copy the correct string with the result of the mememory comparison
                if(strcmp(cmpReturn,"Null")==0) {
                    memCmp = 0;
                    strcpy(Self_MemInconsistency,"No");
                } else {
                    memCmp = 1;
                    strcpy(Self_MemInconsistency,"Yes");
                }

                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                ///Register final state comparison
                sprintf(DumpName,"%s/%s-%d-%d",dumpDirectory,faultRegisterDumpFilesPrefix,processorNumber,PlatformIdentifier);
                sprintf(GoldName,"%s/%s-%d",dumpDirectory,goldRegisterDumpFilesPrefix,0);

                ///Dump File
                icmProcessorSaveStateFile(PE.processorObj,DumpName);
                ///Cut the header
                switch(NumberOfRunningCores) {
                    case 1:
                        sprintf(temp," sed -i \'1,17d\' %s",DumpName); break;
                    case 2:
                        sprintf(temp," sed -i \'1,28d\' %s",DumpName); break;
                    case 4:
                        sprintf(temp," sed -i \'1,50d\' %s",DumpName); break;
                }
                systemReturn=system(temp);

                ///Compare two text files
                strcpy(cmpReturn,"Null");
                sprintf(temp,"cmp %s %s ",DumpName,GoldName);
                printf("%s\n",temp);
                pFileGold = popen(temp, "r");
                fgetsReturn=fgets(cmpReturn,STRING_SIZE,pFileGold);
                pclose(pFileGold);

                ///Remove Dump file to reduce the HD footprint
                if(removeDumpFiles) {
                    sprintf(temp,"rm %s",DumpName);
                    systemReturn=system(temp);
                }

                if(strcmp(cmpReturn,"Null")==0)
                    regCmp = 0;
                else
                    regCmp = 1;

                printf(""FMT_64u" "FMT_64u" \n",Count,GoldCount);
                if(Count != GoldCount) {
                    if(!memCmp)
                        code=Control_Flow_Data_OK;
                    else
                        code=Control_Flow_Data_ERROR;
                ///Internal State
                }else if(regCmp) {
                    if(!memCmp)
                        code=REG_STATE_Data_OK;
                    else
                        code=REG_STATE_Data_ERROR;
                ///SDC
                }else if(memCmp) {
                    code=SDC;
                ///Masked fault
                }else { code=Masked_Fault;}

        ///An exception was detected during the simulation
        }else if (PE.StopReason == ICM_SR_RD_PRIV)      { code = RD_PRIV;
        }else if (PE.StopReason == ICM_SR_WR_PRIV)      { code = WR_PRIV;
        }else if (PE.StopReason == ICM_SR_RD_ALIGN)     { code = RD_ALIGN;
        }else if (PE.StopReason == ICM_SR_WR_ALIGN)     { code = WR_ALIGN;
        }else if (PE.StopReason == ICM_SR_FE_PRIV)      { code = FE_PRIV;
        }else if (PE.StopReason == ICM_SR_FE_ABORT)     { code = FE_ABORT;
        }else if (PE.StopReason == ICM_SR_ARITH)        { code = ARITH;
        }else if (PE.StopReason == FIM_SR_Hard_Fault)   { code = Hard_Fault;
        }else if (PE.StopReason == FIM_SR_Hang)         { code = Hang;
        }else if (PE.StopReason == FIM_SR_Lockup)       { code = Lockup;
        }else if (PE.StopReason == FIM_SR_SEG_FAULT)    { code = SEG_FAULT;
        }else if (PE.StopReason == FIM_SR_Masked)       { code = Masked_Fault;}

        //~ printf("Code: %d REG: %d MEM: %d\n",code,regCmp,memCmp);

        ///Prepare to write in the file
        sprintf(self_faultIndex,"%d",PE.faultIdentifier);
        sprintf(self_faultType,"REGISTER");
        sprintf(self_faultRegister,"%s",PE.faultRegister);
        sprintf(self_faultRegisterIndex,"%u",PE.faultRegisterPosition);
        sprintf(self_faultTime,""FMT_64u"",PE.faultPos);
        sprintf(self_faultMask,"%X",PE.faultValue);
        sprintf(self_faultAnalysis_name,"%s",errorNames[code]);
        sprintf(self_faultAnalysis_value,"%d",code);
        sprintf(self_ExecutedTicks,"0"); //Always zero in the OVP
        sprintf(self_ExecutedInstructions,""FMT_64u"",TotalExecInst);
        //sprintf(Self_MemInconsistency,"%s");
        sprintf(self_correctCheckpoint,""FMT_64u"",CheckpointLoaded+PE.instructionsToNormalExit);

        fprintf(pFile,"%7s %10s %6s %4s %15s %12s %25s %7s %20s %22s %14s %18s\n",
                    self_faultIndex,
                    self_faultType,
                    self_faultRegister,
                    self_faultRegisterIndex,
                    self_faultTime,
                    self_faultMask,
                    self_faultAnalysis_name,
                    self_faultAnalysis_value,
                    self_ExecutedTicks,
                    self_ExecutedInstructions,
                    Self_MemInconsistency,
                    self_correctCheckpoint
                    );
    }
    fclose (pFile);
}
}

void updateStopReason(int stopReason,int processorNumber) {
    PE.StopReason=stopReason;
}

/** Functions - initialization **/
/** ------------------------------------------------------------------------------------- **/
void parseArguments(int argc, char **argv) {
    using optparse::OptionParser;
    OptionParser parser = OptionParser();

    ///Options
    parser.add_option("--FIM_gold_run")          .dest("FIM_gold_run")          .action("store_true").help("Gold run to acquire fundamental information about the application");
    parser.add_option("--FIM_fault_campaign_run").dest("FIM_fault_campaign_run").action("store_true").help("Inject faults");

    parser.add_option("--FIM_ARCH")              .dest("FIM_ARCH")              .action("store")    .type("string")                                     .help("Specified architecture");
    parser.add_option("--FIM_Application")       .dest("FIM_Application")       .action("store")    .type("string")                                     .help("Application binary file");
    parser.add_option("--FIM_project_folder")    .dest("FIM_project_folder")    .action("store")    .type("string")                                     .help("Project folder");
    parser.add_option("--FIM_fault_list")        .dest("FIM_fault_list")        .action("store")    .type("string").set_default("./FAULT_list.FIM_log") .help("File containign the fault list");
    parser.add_option("--FIM_platform_ID")       .dest("FIM_platform_ID")       .action("store")    .type("int")   .set_default(-1)                     .help("Unique platform identifier default: %default");
    parser.add_option("--FIM_number_of_faults")  .dest("FIM_number_of_faults")  .action("store")    .type("int")   .set_default(1)                      .help("Number of faults to be injected in this platform default, equal the number of running processors: %default");
    parser.add_option("--FIM_checkpoint_profile").dest("FIM_checkpoint_profile").action("store")    .type("int")   .set_default(0)                      .help("Create periodical checkpoints during the gold execution and use to speedup the Fault Injection: %default");
    parser.add_option("--FIM_application_argv")  .dest("FIM_application_argv")  .action("store")    .type("string")                                     .help("Argument pass to the application");

    //~ char const* const platformChoices[] = {"BareMetal", "Linux"}; //Create choices
    parser.add_option("--FIM_Mode").dest("FIM_Mode").action("store").type("string").set_default("BareMetal").help("...");

    ///Linux Options
    parser.add_option("--FIM_linux_bootloader") .dest("FIM_linux_bootloader")   .action("store").        type("string").help("Specified the linux bootloader");
    parser.add_option("--FIM_linux_graphics")   .dest("FIM_linux_graphics")     .action("store")        .type("string").help("linux graphical interface");
    parser.add_option("--FIM_enable_console")   .dest("FIM_enable_console")     .action("store_true")                  .help("enable pop window console");
    parser.add_option("--FIM_number_of_Cores")  .dest("FIM_number_of_Cores")    .action("store")        .type("int")   .help("Number of target cores").set_default(1);
    parser.add_option("--FIM_linux_folder")     .dest("FIM_linux_folder")       .action("store")        .type("string").help("linux image folder");

    options = parser.parse_args(argc, argv);
    vector<string> args = parser.args();

    if(IsBareMetalMode) {
           if (!options["FIM_ARCH"].compare("ARM_CORTEX_A5")) { strcpy(modelVariant,"Cortex-A5UP");
    } else if (!options["FIM_ARCH"].compare("ARM_CORTEX_A8")) { strcpy(modelVariant,"Cortex-A8");
    } else if (!options["FIM_ARCH"].compare("ARM_CORTEX_A7")) { strcpy(modelVariant,"Cortex-A7UP");
    } else if (!options["FIM_ARCH"].compare("ARM_CORTEX_A9")) { strcpy(modelVariant,"Cortex-A9UP");
    } else if (!options["FIM_ARCH"].compare("ARM_CORTEX_A15")){ strcpy(modelVariant,"Cortex-A15UP");
    } else if (!options["FIM_ARCH"].compare("ARM_CORTEX_M3")) { strcpy(modelVariant,"Cortex-M3");
    } else if (!options["FIM_ARCH"].compare("ARM_CORTEX_M4")) { strcpy(modelVariant,"Cortex-M4F");
    } else if (!options["FIM_ARCH"].compare("MIPS32"))        { strcpy(modelVariant,"ISA");
    }} else if (IsLinuxMode) {
           if (!options["FIM_ARCH"].compare("ARM_CORTEX_A9")) { strcpy(modelVariant,"Cortex-A9");
    } else if (!options["FIM_ARCH"].compare("ARM_CORTEX_A15")){ strcpy(modelVariant,"Cortex-A15");
    }

    sprintf(modelVariant,"%sMPx%d",modelVariant,(int)options.get("FIM_number_of_Cores"));
    printf("Variant %s \n",modelVariant);
    }

}

void initPlatform() {
    ///Allocate the array according with the number of DUT
    vectorProcessor = (struct processorStruct*) malloc( sizeof(struct processorStruct) * NumberOfRunningFaults);

    ///Automatically count the number of possible register in the list --Review -> Get from the OVP
    NumberOfPossibleRegisters=0;
    while(strcmp(possibleRegisters[NumberOfPossibleRegisters],END_LIST))
        NumberOfPossibleRegisters++;
}

void initProcessorInstFIM(Uns32 processorNumber, icmProcessorP processorObj, icmMemoryP  memObj) {

    ///Define PE id
    PE.id = processorNumber;

    ///Get PE handlers for the processor and memory object
    PE.processorObj=processorObj;
    PE.memObj=memObj;

    ///Initialise the PE finished status
    PE.StopReason=FIM_SR_Unset;

    ///Calculate the number of instructions saved by the early termination
    PE.instructionsToNormalExit=0;

    ///After kernel point
    PE.ApplicationBegan=0;

    ///Allocate the arrays
    PE.childrens        = (icmProcessorP*)  calloc (NumberOfRunningCores ,sizeof(icmProcessorP));
    PE.expectedICount   = (Uns64*)          calloc (NumberOfRunningCores ,sizeof(Uns64));
    PE.ApplicationBegan = (Uns64*)          calloc (NumberOfRunningCores ,sizeof(Uns64));
    PE.ApplicationEnd   = (Uns64*)          calloc (NumberOfRunningCores ,sizeof(Uns64));

    if(IsBareMetalMode) { /// !!!! Only valid to one core processor
        PE.childrens[0]=PE.processorObj;
    } else {
        ///First core
        PE.childrens[0]=icmGetSMPChild(PE.processorObj);

        ///Get all cores handlers
        for (int i=1;i<NumberOfRunningCores;i++)
            PE.childrens[i]=icmGetSMPNextSibling(PE.childrens[i-1]);
    }


    ///Instantiate the FIM intercept
    //~ char temp[256];
    //~ icmAttrListP icmAttr = icmNewAttrList();
    //~ icmAddPtrAttr(icmAttr,"vectorProcessor",(void*) &vectorProcessor[processorNumber]);
    //~ sprintf(temp,"%s/Platform/InterceptLib/model.so",ApplicationFolder);
    //~ icmAddInterceptObject(processorObj,"intercept_com",temp, NULL,icmAttr);

    if(IsFaultCampaignExecution) {
        ///Open a file contaning the gold count
        char temp[256];
        FILE * pFileGold;

        for (int i=0;i<NumberOfRunningCores;i++) {
            ///Reads the gold count for each core
            sprintf(temp,"echo $(grep \"Executed Instructions Per core=\" ./Information.FIM_log | cut -d '=' -f2 | cut -d ',' -f%d)",i+1);
            pFileGold = popen(temp, "r");
            fgetsReturn=fgets(temp,STRING_SIZE,pFileGold);
            sscanf(temp,""FMT_64u"",&PE.expectedICount[i]);
            printf(""FMT_64u"\n",PE.expectedICount[i]);
            pclose(pFileGold);

            ///Reads the application began for each core
            sprintf(temp,"echo $(grep \"Application begins=\" ./Information.FIM_log | cut -d '=' -f2 | cut -d ',' -f%d)",i+1);
            pFileGold = popen(temp, "r");
            fgetsReturn=fgets(temp,STRING_SIZE,pFileGold);
            sscanf(temp,""FMT_64u"",&PE.ApplicationBegan[i]);
            //~ printf(""FMT_64u"\n",PE.ApplicationBegan[i]);
            pclose(pFileGold);
        }
    }
}

/** ------------------------------------------------------------------------------------- **/
/**                                     Functions - Fault Injection                       **/
void setProfiling(Uns32 processorNumber) { /// !!!! Only valid to one core processor
    if(IsCheckpointEnable) {
        ///Used to configure the interval between checkpoints
        PE.expectedICount[0]=CheckpointInterval;
        ///Set the first checkpoint
        icmSetICountBreakpoint(PE.processorObj,CheckpointInterval);

        PE.Status=GOLD_INJECTION;
        ///Create the checkpoint ZERO
        icmProcessorSaveStateFile(PE.processorObj,"./Checkpoints/chckReg0");
        icmMemorySaveStateFile(PE.memObj,"./Checkpoints/chckMem0");

        icmMemoryRestoreStateFile(PE.memObj,"./Checkpoints/chckMem0");
        icmMemorySaveStateFile(PE.memObj,"./Checkpoints/chckMem0");
    }
}

void loadFaults() {
    int i;
    int processorNumber;
    char temp[MAX_TAM_LIN];
    char chckName[STRING_SIZE];

    FILE *fileptr  = fopen (options["FIM_fault_list"].c_str(),"r");

    if (fileptr == NULL) {
        printf("Fault list file can't be used \n ");
        exit(-1);
    }

    ///Skip header file
    fgetsReturn=fgets(temp,MAX_TAM_LIN,fileptr);

    ///Getting the right file possition
    for(i=0;i < (PlatformIdentifier-1)*NumberOfRunningFaults ;i++)
        fgetsReturn=fgets(temp,MAX_TAM_LIN,fileptr);

    int core = 0;
    ///Load the faults
    for(processorNumber=0;processorNumber<NumberOfRunningFaults;processorNumber++) {

        ///Get one line
        fgetsReturn=fgets(temp,MAX_TAM_LIN,fileptr);
        ///Extract the info
        sscanf(temp,"%d %*s %*s %d "FMT_64u":%d %X \n",&PE.faultIdentifier,&PE.faultRegisterPosition,&PE.faultPos,&core,&PE.faultValue);

        ///Core to inject
        PE.coreToInject=PE.childrens[core];

        ///REVIEW
        PE.faultType = REGISTER_FAULT;
        PE.faultDuration = 1;

        if(PE.faultType==REGISTER_FAULT) {
            strcpy(PE.faultRegister,possibleRegisters[PE.faultRegisterPosition]);
        } else  if(PE.faultType==MEMORY_FAULT) {
            strcpy(PE.faultRegister,"MEM");
        } else  if(PE.faultType==REG_FAULT_TEMPORARY) {
            strcpy(PE.faultRegister,possibleRegisters[PE.faultRegisterPosition]);
        }

        ///PE status
        PE.Status=BEFORE_INJECTION;
        printf("Loaded Fault %d "FMT_64u" 0x%X %d %s core %d\n",PE.faultIdentifier,PE.faultPos,PE.faultValue,PE.faultRegisterPosition,PE.faultRegister,core);

        ///Adjusting the fault to use checkpoint
        if(IsCheckpointEnable) { /// !!!! Only valid to one core processor
            PE.CHCKpoint=PE.faultPos/CheckpointInterval;

            sprintf(chckName,"./Checkpoints/chckReg"FMT_64u,PE.CHCKpoint);
            icmProcessorRestoreStateFile(PE.processorObj,chckName);

            sprintf(chckName,"./Checkpoints/chckMem"FMT_64u,PE.CHCKpoint);
            icmMemoryRestoreStateFile(PE.memObj,chckName);

            ///Adjust Fault the absolute position to a relative position
            PE.faultPos=PE.faultPos - (PE.CHCKpoint*CheckpointInterval);

            printf("Relative position "FMT_64u" # Checkpoint "FMT_64u"\n",PE.faultPos,PE.CHCKpoint);
            PE.CHCKCount=0;
        }

        ///Set Breakpoint
        icmSetICountBreakpoint(PE.coreToInject,PE.faultPos);

    }
    fclose(fileptr);
}

/** Release all cores in multicore or single core platforms**/
void YieldAllCores(Uns32 processorNumber) {
    for (int i=0;i<NumberOfRunningCores;i++)
        icmFreeze(PE.childrens[i]); //Retrieves from the simulator scheduler

    //Yield signal to stop simulation
    icmYield(PE.processorObj);
}

/** Functions called by the CallBacks **/
void GoldProfiling(Uns32 processorNumber) {  /// !!!! Only valid to one core processor
    if(IsCheckpointEnable) {
    char chckName[STRING_SIZE];

        printf("ICount "FMT_64u"\n",icmGetProcessorICount(PE.processorObj));

        ///Configure next checkpoits
        ///Clear the older ICountBreakpoint (not necessary, but for precaution)
        //icmClearICountBreakpoint(PE.processorObj);
        ///Set next ICountBreakpoint
        icmSetICountBreakpoint(PE.processorObj,CheckpointInterval);

        Uns64 tempCount = (Uns64) (icmGetProcessorICount(PE.processorObj)/CheckpointInterval);

        ///Create the checkpoint
        printf("Chck "FMT_64u"\n",tempCount);
        sprintf(chckName,"./Checkpoints/chckReg"FMT_64u"",tempCount);
        icmProcessorSaveStateFile(PE.processorObj,chckName);

        icmFlushMemory(PE.memObj,BASE_MEMORY,SIZE_MEMORY);
        sprintf(chckName,"./Checkpoints/chckMem"FMT_64u"",tempCount);
        icmMemorySaveStateFile(PE.memObj,chckName);
    }
}

void FaultInjection(Uns32 processorNumber) {
    int regTemp;
    char chckName[STRING_SIZE];
    char DumpName[STRING_SIZE];
    char cmpReturn[STRING_SIZE];
    char temp[STRING_SIZE];
    FILE * pFileGold;

    switch(PE.Status) {
        case BEFORE_INJECTION: { ///Inject the fault
            Uns64 tempCount;
            PE.Status = AFTER_INJECTION;

            /// Define the Hang condition
            if(IsCheckpointEnable) { /// !!!! Only valid to one core processor
                if(PE.expectedICount[0] - icmGetProcessorICount(PE.processorObj) < (Uns64) (2*CheckpointInterval) ){ ///Configure the wait for Hang
                    ///Configure the next ICountBreakpoint to signal Hangs
                    //icmClearICountBreakpoint(PE.processorObj);
                    tempCount = (Uns64) ((PE.expectedICount[0] - icmGetProcessorICount(PE.processorObj)) + PE.expectedICount[0]*FIM_HANG_THRESHOLD);
                    icmSetICountBreakpoint(PE.processorObj,tempCount);
                    PE.Status = WAIT_HANG;
                    icmFlushMemory(PE.memObj,BASE_MEMORY,SIZE_MEMORY);
                } else { ///Wait NEXT CHECKPOINT
                    ///Configure the next ICountBreakpoint
                    //icmClearICountBreakpoint(PE.processorObj); //Clear the older ICountBreakpoint
                    ///Gets the actual Icount and calculte the next ICountBreakpoint
                    tempCount = (Uns64) ( CheckpointInterval - icmGetProcessorICount(PE.processorObj) % CheckpointInterval);
                    ///Set the next breakpoint
                    icmSetICountBreakpoint(PE.processorObj,tempCount);
                }
            } else {
                ///Set the wait to hang for all cores -- including baremetal
                for (int i=0;i<NumberOfRunningCores;i++) {
                    tempCount = (Uns64) ( ( PE.expectedICount[i] - icmGetProcessorICount(PE.childrens[i]) ) + ((PE.expectedICount[i] - PE.ApplicationBegan[i])*FIM_HANG_THRESHOLD) );
                    icmSetICountBreakpoint(PE.childrens[i],tempCount);
                }

                PE.Status = WAIT_HANG;
            }

            ///Insert the fault
            if(PE.faultType==REGISTER_FAULT) {
                    printf("Reg %3s ",PE.faultRegister);
                icmReadReg(PE.coreToInject,PE.faultRegister,&regTemp);
                    printf("Old 0x%8X ",regTemp);
                regTemp= ~(regTemp ^ PE.faultValue);
                    printf("New 0x%8X\n",regTemp);
                icmWriteReg(PE.coreToInject,PE.faultRegister,&regTemp);
            }
            /*else if(PE.faultType==MEMORY_FAULT) {
                char temps;
                icmReadProcessorMemory(PE.processorObj,PE.faultRegisterPosition,&temps,1);
                temps= ~(temps ^ PE.faultValue);
                icmWriteProcessorMemory(PE.processorObj,PE.faultRegisterPosition, &temps,1);

                //icmFlushMemory(PE.memObj,0x0,SIZE_MEMORY);
                Uns32 tempAddress = PE.faultRegisterPosition-(PE.faultRegisterPosition%0x40);
                printf("0X%X 0X%X \n",tempAddress,tempAddress+0x3F);
                icmFlushProcessorMemory(PE.processorObj,tempAddress,tempAddress+0x3F);

            } else if(PE.faultType==REG_FAULT_TEMPORARY) {
                icmSetRegisterWatchPoint(PE.processorObj, icmGetRegByName(PE.processorObj, PE.faultRegister), (void *) processorNumber, 0);
            }*/

      break; }
        case AFTER_INJECTION: { /// Verify if the fault caused an error
            if(IsCheckpointEnable) { /// !!!! Only valid to one core processor baremetal
                int memCmp=0;
                int regCmp=0;
                Uns64 tempCount;

                ///Checkpoint counter
                PE.CHCKCount++;

                ///Configure the next ICountBreakpoint
                ///Clear the older ICountBreakpoint
                //icmClearICountBreakpoint(PE.processorObj);
                ///Gets the actual Icount and calculte the next ICountBreakpoint
                tempCount = (Uns64) (CheckpointInterval - icmGetProcessorICount(PE.processorObj) % CheckpointInterval);
                ///Set the next breakpoint
                icmSetICountBreakpoint(PE.processorObj,tempCount);
                tempCount = (Uns64) (icmGetProcessorICount(PE.processorObj)/CheckpointInterval);

                ///Compare the processor state
                sprintf(DumpName,"./Dumps/RegDump%u-%u-"FMT_64u"",PlatformIdentifier,processorNumber,tempCount);
                sprintf(chckName,"./Checkpoints/chckReg"FMT_64u"",tempCount);
                icmProcessorSaveStateFile(PE.processorObj,DumpName);

                sprintf(temp,"cmp %s %s ",chckName,DumpName);
                strcpy(cmpReturn,"Null");
                pFileGold = popen(temp, "r");
                fgetsReturn=fgets(cmpReturn,STRING_SIZE,pFileGold);

                ///Remove Dump file to reduce the HD footprint
                sprintf(temp,"rm %s",DumpName);
                systemReturn=system(temp);

                printf("Comparing if the fault was masked \n");
                printf("REG %s \n",temp);

                icmFlushMemory(PE.memObj,BASE_MEMORY,SIZE_MEMORY);

                ///Compare the popen return with the previous state, if its equal means that the files are equal (i.e. the cmp command didn't return anything)
                if(strcmp(cmpReturn,"Null")==0)
                    regCmp = 0;
                else
                    regCmp = 1;

                ///Compare the memory state
                sprintf(DumpName,"./Dumps/MemDump%u-%u-"FMT_64u"",PlatformIdentifier,processorNumber,tempCount);
                sprintf(chckName,"./Checkpoints/chckMem"FMT_64u"",tempCount);
                icmMemorySaveStateFile(PE.memObj,DumpName);

                sprintf(temp,"cmp %s %s ",chckName,DumpName);
                strcpy(cmpReturn,"Null");
                pFileGold = popen(temp, "r");
                fgetsReturn=fgets(cmpReturn,STRING_SIZE,pFileGold);

                ///Remove Dump file to reduce the HD footprint
                sprintf(temp,"rm %s",DumpName);
                systemReturn=system(temp);

                ///Compare the popen return with the previous state, if its equal means that the files are equal (i.e. the cmp command didn't return anything)
                if(strcmp(cmpReturn,"Null")==0)
                    memCmp = 0;
                else
                    memCmp = 1;

                printf("MEM %s \n",temp);
                printf("Result %d %d\n",regCmp,memCmp);

                ///No change in the context - Fault was masked
                if( regCmp==0 && memCmp==0) {
                    printf("Fault masked\n");
                    PE.instructionsToNormalExit=PE.expectedICount[0] - icmGetProcessorICount(PE.processorObj);
                    updateStopReason(FIM_SR_Masked,processorNumber);
                    YieldAllCores(processorNumber);
                }

                ///Second count arrived, now wait for the hang
                if((PE.CHCKCount == 2) || ( (PE.expectedICount[0] - icmGetProcessorICount(PE.childrens[0])) < (Uns64) (2*CheckpointInterval) ) ) {
                    //Configure the next ICountBreakpoint to signal Loops
                    //icmClearICountBreakpoint(PE.processorObj);
                    tempCount = (Uns64) ((PE.expectedICount[0] - icmGetProcessorICount(PE.childrens[0])) + PE.expectedICount[0]*FIM_HANG_THRESHOLD);
                    icmSetICountBreakpoint(PE.childrens[0],tempCount);
                    PE.Status = WAIT_HANG;
                }
            }
        break;
        }
        case WAIT_HANG: { ///Wait for one processor to hang
            printf("LOOP detected "FMT_64u"\n",icmGetProcessorICount(PE.processorObj));
            updateStopReason(FIM_SR_Hang,processorNumber);
            YieldAllCores(processorNumber);
        break; }

    }
}

/** Exception only in M4-M0 **/
void LockupExcepetion (void *arg, Uns32 value) {
    int processorNumber = (intptr_t) arg;
    printf("Lookup Excepetion detected!!! %d\n",processorNumber );
    updateStopReason(FIM_SR_Lockup,processorNumber);
    YieldAllCores(processorNumber);
}

///Used for forcing a value --Not in use and need review to multicore
void handleWatchpoints(void) {
    icmWatchPointP wp;
    int regTemp;

    while((wp=icmGetNextTriggeredWatchPoint())) {

    int processorNumber     = (intptr_t) icmGetWatchPointUserData(wp);
    icmProcessorP processor = icmGetWatchPointTriggeredBy(wp);


            /// a register watchpoint was triggered
            icmRegInfoP reg       = icmGetWatchPointRegister(wp);
            Uns32      *newValueP = (Uns32 *)icmGetWatchPointCurrentValue(wp);
            Uns32      *oldValueP = (Uns32 *)icmGetWatchPointPreviousValue(wp);

            /// indicate old and new value of the affected register
            icmPrintf(
                "watchpoint %u (processor %s:%s) triggered 0x%08x->0x%08x\n",
                processorNumber,
                icmGetProcessorName(processor, "/"),
                icmGetRegInfoName(reg),
                *oldValueP,
                *newValueP
            );

            icmResetWatchPoint(wp);

            icmReadReg(PE.processorObj,PE.faultRegister,&regTemp);
            regTemp= ~(regTemp ^ PE.faultValue);
            icmWriteReg(PE.processorObj,PE.faultRegister,&regTemp);
            icmPrintf(" Value 0x%08x\n",regTemp);

    }
}

ICM_MEM_WATCH_FN(faultInjector) {
    int processorNumber = (intptr_t) userData; ///Inform the id of the processor wich triggered the callback
    printf("Address 0x%X\n", (int) address);

    updateStopReason(FIM_SR_Unset,processorNumber);
    unsigned int code = (unsigned int) address - baseAddress;

    if(IsBareMetalMode) { /// !!!! Only valid to one core processor
                if (code == FIM_SERVICE_END_APP)   {
            printf("Appllication ending at ICOUNT "FMT_64u"\n",icmGetProcessorICount(PE.processorObj));
            updateStopReason(FIM_SR_Unset,processorNumber);
            YieldAllCores(processorNumber);
        } else  if (code == FIM_SERVICE_BEGIN_APP) {
            if(IsGoldExecution) { ///Only necessary in the gold run
                PE.ApplicationBegan[0]= icmGetProcessorICount(PE.processorObj);
            }
        }
    } else {
                if (code == FIM_SERVICE_END_APP)   {
                    printf("Application finished:");
                    for (int i=0;i<NumberOfRunningCores;i++) {
                        printf(""FMT_64u" ",icmGetProcessorICount(PE.childrens[i]));
                        PE.ApplicationEnd[i]=icmGetProcessorICount(PE.childrens[i]);
                    }
                    printf("\n");
                    updateStopReason(FIM_SR_Unset,processorNumber);
        } else  if (code == FIM_SERVICE_BEGIN_APP) { ///Only necessary in the gold run
                    printf("Application began:");
                    for (int i=0;i<NumberOfRunningCores;i++) {
                        printf(""FMT_64u" ",icmGetProcessorICount(PE.childrens[i]));
                        PE.ApplicationBegan[i]=icmGetProcessorICount(PE.childrens[i]);
                        printf("\n");
                    }
        } else  if (code == FIM_SERVICE_SEG_FAULT) {
                    printf("Appllication ending after a segmentation fault at ICOUNT "FMT_64u" \n",icmGetProcessorICount(PE.processorObj));
                    updateStopReason(FIM_SR_SEG_FAULT,processorNumber);
                    YieldAllCores(processorNumber);
        } else  if (code == FIM_SERVICE_EXIT_APP) {
                    printf("Application finishs-");
                    YieldAllCores(processorNumber);
        } else {
                    printf("Unidentified cause \n");
                    updateStopReason(FIM_SR_UNIDENTIFIED,processorNumber);
                    YieldAllCores(processorNumber);
        }
    }
}


