#ifndef _Classification
#define _Classification

/*
 * Fault Injection Version - 1.01
 */

//Memory definition
#define BASE_MEMORY 0X00000000
//#define SIZE_MEMORY 0X08000000 //256mb
#define SIZE_MEMORY 0x3fffffff //256mb


#define STRING_SIZE 256
#define MAX_TAM_LIN 300 //Size of one line file get
#define END_LIST "EndList@"


//Files
const char dumpDirectory []                 = "./Dumps";
const char faultMemoryDumpFilesPrefix []    = "FAULT_memory_PE";
const char faultRegisterDumpFilesPrefix []  = "FAULT_register_PE";
const char goldMemoryDumpFilesPrefix []     = "GOLD_memory_PE";
const char goldRegisterDumpFilesPrefix []   = "GOLD_register_PE";

//Fault List
//const char faultListDirectory []          ="./Faults";
const char faultListPrefix []               ="FAULT_list";

#ifdef MIPS32
    const char possibleRegisters[][STRING_SIZE] ={"at","v0","v1","a0","a1","a2","a3","t0","t1","t2","t3","t4","t5","t6","t7","s0","s1","s2","s3","s4","s5","s6","s7","t8","t9","k0","k1","gp","sp","s8","ra","pc",END_LIST};
#else
    const char possibleRegisters[][STRING_SIZE] ={"r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12","sp","lr","pc",END_LIST};
#endif

//Types of errors
enum {
    REGISTER_FAULT,
    MEMORY_FAULT,
    REG_FAULT_TEMPORARY
};


//Final error classification
enum {
    Masked_Fault ,                  //Masked fault
    Control_Flow_Data_OK,           //Divergency between executed insctructions and gold, memory correct
    Control_Flow_Data_ERROR,        //Divergency between executed insctructions and gold, memory incorrect
    REG_STATE_Data_OK,              //Internal state incorrect and memory no
    REG_STATE_Data_ERROR,           //Internal state and memory incorrect
    SDC,                            //Silent Data Corruption
    RD_PRIV,                        //Read privilege exception.
    WR_PRIV,                        //Write privilege exception.
    RD_ALIGN,                       //Read align exception.
    WR_ALIGN,                       //Write align exception.
    FE_PRIV,                        //Fetch privilege exception.
    FE_ABORT,                       //Fetch an inconsistent instruction
    SEG_FAULT,
    ARITH,                          //Arithmetic exception.
    Hard_Fault,                     //Hard Fault handler
    Lockup,                         //Cortex M4 special state
    Unknown,                        //Unknown
    Hang,                           //Application presumably hanged
    SIZE_ERROR_CLASS_LIST           //Size of this list
};

//The name of the error classes, only keep the same order
const char* const errorNames[] = {
    "Masked_Fault",             
    "Control_Flow_Data_OK",     
    "Control_Flow_Data_ERROR",  
    "REG_STATE_Data_OK",        
    "REG_STATE_Data_ERROR",     
    "silent_data_corruption",   
    "RD_PRIV",                  
    "WR_PRIV",                  
    "RD_ALIGN",                 
    "WR_ALIGN",                 
    "FE_PRIV",                  
    "FE_ABORT",                 
    "SEG_FAULT",                
    "ARITH",                    
    "Hard_Fault",               
    "Lockup",                   
    "Unknown",                  
    "Hang",                     
    "SIZE_ERROR_CLASS_LIST"};   

#endif
