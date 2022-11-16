#ifndef _FIM
#define _FIM

void fimSimulate(struct optionsS _options, optModuleP _mi);
void initProcessorInstFIM(Uns32 processorNumber);
void removeFromScheduler(Uns32 processorNumber);
void initialize();

void dumpGoldInformation();

void report();

void handlerFaultInjection(Uns32 processorNumber);
void handlerGoldProfiling (Uns32 processorNumber);
void applicationRequestHandler(int processorNumber);

void createFaultInjectionReport();

void loadFaults();
void setProfiling ();

void LockupExcepetion (void *arg, Uns32 value);
void handleWatchpoints(void);
OP_MONITOR_FN(UI_faultInjector);

//~ void updateStopReason(Uns32 stopReason,Uns32 processorNumber);
//~ void setFault(Uns32 faultType, Uns32 processorNumber);
#endif
