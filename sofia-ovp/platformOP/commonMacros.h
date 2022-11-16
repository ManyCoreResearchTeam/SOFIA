//Parser Options
#define MACRO_GOLD_PLATFORM_FLAG            options.goldrun
#define MACRO_FI_PLATFORM_FLAG              options.faultcampaignrun
#define MACRO_LINUX_GRAPHICS_FLAG           options.linuxgraphics
#define MACRO_LINUX_CONSOLE_FLAG            options.enableconsole
#define MACRO_CHECKPOINT_ENABLE_FLAG       (options.checkpointinterval!=0)
#define MACRO_LINUX_MODE_FLAG       !strcmp(options.mode,"linux")
#define MACRO_LINUX64_MODE_FLAG     !strcmp(options.mode,"linux64")
#define MACRO_BAREMETAL_MODE_FLAG   !strcmp(options.mode,"baremetal")
#define MACRO_BIGLITTLE_FLAG        !strcmp(options.arch,"bigLITTLE")


#define MACRO_PLATFORM_ID                   options.platformid
#define MACRO_NUMBER_OF_FAULTS              options.numberoffaults
#define MACRO_NUMBER_OF_CORES               options.targetcpucores
#define MACRO_CHECKPOINT_INTERVAL           options.checkpointinterval
#define MACRO_PROJECT_FOLDER                options.projectfolder
#define MACRO_APPLICATION_FOLDER            options.applicationfolder

#define FIM_HANG_THRESHOLD 0.5

// Files @Review
#define SUFIX_FIM_TEMP_FILES        "FIM_log" //Temporary files sufix
#define FILE_GOLD_INFORMATION       "goldinformation"
#define FILE_FAULT_LIST             "faultlist"


#define FILE_NAME_MEM_FAULT         "FAULT_MEM"
#define FILE_NAME_REG_FAULT         "FAULT_REG"
#define FILE_NAME_MEM_GOLD          "GOLD_MEM"
#define FILE_NAME_REG_GOLD          "GOLD_REG"

#define FILE_NAME_REG_CHECKPOINT    "CHCK_REG"
#define FILE_NAME_MEM_CHECKPOINT    "CHCK_MEM"

#define FILE_NAME_REG_DUMP          "CHCK_DUMP"
#define FILE_NAME_MEM_DUMP          "CHCK_DUMP"

#define FOLDER_DUMPS                "Dumps"
#define FOLDER_CHECKPOINTS          "Checkpoints"
#define FOLDER_REPORTS              "Reports"

/// messages prefix
#define PREFIX_FIM                  "FIM_TOOL"
#define PREFIX_FIM_TRACE_FUNCTION   "FIM_TOOL_TRACE_FUNCTION"

/// trace output files
#define FILE_NAME_TRACE             "trace.log"

