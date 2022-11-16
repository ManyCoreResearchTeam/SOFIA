/*
"""
Fault Injection Module to Gem5
Version 1.0
02/02/2016

Author: Felipe Rocha da Rosa - frdarosa.com
"""
 */

#include "params/faultInjectionSystem.hh"
#include "fim/faultInjectionSystem.hh"
using namespace std;

faultInjectionSystem *fi_system;
/**
 * Class Creator
 */
faultInjectionSystem::faultInjectionSystem(const Params *p)
   : SimObject(p)
   //faultInjectionEvent(this)
{
    ///Cpu type
    cpuType=p->cpuType;

    /// Mem type
    pmem = p->mem;
    (void) pmem;
    ///Get the correct context
    switch(cpuType) {
     case 2: //Detailed armv7
        ///O3 CPU
        coreDerivO3CPU = (DerivO3CPU*) p->system;
        context = coreDerivO3CPU->tcBase(0);
        numberOfIntegerRegisters=ARMV7_NREGS;
        break;
     case 1: //Timing armv7
     case 0: //Atomic armv7
        ///Cpu object using polymorphism
        core = (BaseCPU*) p->system;
        context =  core->getContext(0);
        numberOfIntegerRegisters=ARMV7_NREGS;
        break;
     case 5: //Detailed armv8
        ///O3 CPU
        coreDerivO3CPU = (DerivO3CPU*) p->system;
        context = coreDerivO3CPU->tcBase(0);
        numberOfIntegerRegisters=AARCH64_NREGS;
        break;
     case 4://Timing armv8
     case 3://Atomic armv8
        ///Cpu object using polymorphism
        core = (BaseCPU*) p->system;
        context =  core->getContext(0);
        numberOfIntegerRegisters=AARCH64_NREGS;
        break;
   }
}

/**
 * Setup the fault
 */
void faultInjectionSystem::setupFault(int64_t faultMask, int faultType, int faultRegister, bool targetPC,uint64_t memAddress){
    this->faultMask     = faultMask;
    this->faultType     = faultType;
    this->faultRegister = faultRegister;
    this->targetPC      = targetPC;
    this->memAddress    = memAddress;
}

/**
 * Fault injector handler
 */
#define MESSAGE printf
void faultInjectionSystem::faultInjectionHandler() {
    //Uns64 tempCount;
    TheISA::IntReg readValue;
    TheISA::IntReg writeValue;
    uint8_t memValue;

    MESSAGE("FIM: Fault Injection Handler\n");
    MESSAGE("FIM: %lx \n",faultMask);

    if (TheISA::inUserMode(context))
        MESSAGE("IN USER MODE\n");

    switch(faultType) {
    case 0: { //Register
        if (targetPC) {
                switch(cpuType) {
                 case 2: //ArmV7 Detailed
                                readValue   = context->pcState().instAddr();                            // Read PC
                                writeValue  =~(readValue ^ (0xFFFFFFFF00000000 | faultMask));           // Mask
                                coreDerivO3CPU->pcState(writeValue,0);
                        break;
                 case 1: //ArmV7 Timing
                 case 0: //ArmV7 Atomic
                                readValue   = context->pcState().instAddr();                            // Read PC
                                writeValue  =~(readValue ^ (0xFFFFFFFF00000000 | faultMask));           // Mask
                                context->pcState( (TheISA::PCState) writeValue);                        // Write Back PC
                        break;
                 case 5: //ArmV8 Detailed
                                readValue   = context->pcState().instAddr();                            // Read PC
                                writeValue  =~(readValue ^ faultMask);                                  // Mask
                                coreDerivO3CPU->pcState(writeValue,0);
                        break;
                 case 4: //ArmV8 Timing
                 case 3: //ArmV8 Atomic
                                readValue   = context->pcState().instAddr();                            // Read PC
                                writeValue  =~(readValue ^ faultMask);                                  // Mask
                                context->pcState( (TheISA::PCState) writeValue);                        // Write Back PC
                        break;
           }
        } else {
                switch(cpuType) {
                 case 2: //ArmV7 Detailed
                                readValue   = context->readIntReg(faultRegister);                       // Read
                                writeValue  =~(readValue ^ (0xFFFFFFFF00000000 | faultMask));           // Mask
                                coreDerivO3CPU->setArchIntReg(faultRegister,writeValue,0);              // Write Back
                        break;
                 case 1: //ArmV7 Timing
                 case 0: //ArmV7 Atomic
                                readValue   = context->readIntReg(faultRegister);                       // Read
                                writeValue  =~(readValue ^ (0xFFFFFFFF00000000 | faultMask));           // Mask
                                context->setIntReg(faultRegister,writeValue);                           // Write Back
                        break;
                 case 5: //ArmV8 Detailed
                                readValue   = context->readIntReg(faultRegister);                       // Read
                                writeValue  =~(readValue ^ faultMask);                                  // Mask
                                coreDerivO3CPU->setArchIntReg(faultRegister,writeValue,0);              // Write Back
                        break;
                 case 4: //ArmV8 Timing
                 case 3: //ArmV8 Atomic
                                readValue   = context->readIntReg(faultRegister);                       // Read
                                writeValue  =~(readValue ^ faultMask);                                  // Mask
                                context->setIntReg(faultRegister,writeValue);                           // Write Back
                        break;
           }
        }
        MESSAGE("FIM: Arch %1d Target %2d A:0x%016lX B:0x%016lX Mask:0x%016lX\n",cpuType,faultRegister,readValue,writeValue,faultMask);
        break; }
    case 1: { //Memory
        memValue=pmem->pmemAddr[memAddress];
        pmem->pmemAddr[memAddress]=~(pmem->pmemAddr[memAddress]^ (uint8_t) faultMask);

        MESSAGE("MEM: add %016lx original %02x mod %02x \n",memAddress,pmem->pmemAddr[memAddress],memValue);
        break;}
    }


/*    if (faultType==0) {
        printf("Fault type REG\n");
        printf("PC: 0x%lX MASK: 0x%X REG: %d\n",context->pcState().instAddr(),faultMask,faultRegister);

        if(targetPC) {///Program counter fault injection
            ReadPC()
            ApplyMask()
            /// The O3 detailed model requires a different access function through the CPU obj and not the context obj
            if (cpuType == 0)
                WritePC()
            else
                coreDerivO3CPU->pcState(tempResult,0);

        } else {///Others registers
            ReadRegister()
            ApplyMask()
            /// The O3 detailed model requires a different access function through the CPU obj and not the context obj
            if (cpuType == 0)
                WriteRegister()
            else
                coreDerivO3CPU->setArchIntReg(faultRegister,tempResult,0);

        //~ printf("Before %lX After %lX (%lX) mask %lX\n",tempRegister,tempResult,coreDerivO3CPU->readArchIntReg(faultRegister,0),tempMask);
        //~ printf("Before %lX After %lX mask %lX\n",tempRegister,tempResult,tempMask);
        }
    } else {
        printf("Fault type ROB\n");
        this->template ROBFaultInjector <O3CPUImpl> (faultMask,faultType);
    } */
}

/**
 * Fault injection in the ReOrder Buffer
 */
template <class O3CPUImpl>
void faultInjectionSystem::ROBFaultInjector(const int faultMask, const int faultType) {
    typedef typename O3CPUImpl::DynInstPtr DynInstPtr;
    typedef typename std::list<DynInstPtr>::iterator InstIt;

    //Considering only one thread
    int tid=0, counter=1, position=2;
    long unsigned int seqNum;
    int regSource_1, regSource_2, regDest;

    printf("Rob size %d\n",coreDerivO3CPU->rob.numInstsInROB);

    for (InstIt it = coreDerivO3CPU->rob.instList[tid].begin(); it != coreDerivO3CPU->rob.instList[tid].end(); it++) {
        seqNum = (*it)->seqNum;
        regSource_1 = (*it)->_srcRegIdx[0];
        regSource_2 = (*it)->_srcRegIdx[1];
        regDest     = (*it)->_destRegIdx[0];

        printf("%32s ",(*it)->staticInst->disassemble((*it)->instAddr()).c_str());
        printf("Seq %lu Dest %4d Source1 %4d Source2 %4d\n",seqNum,regDest,regSource_1,regSource_2);
        printf("Seq %lu Dest %4d Source1 %4d Source2 %4d\n",seqNum,(*it)->staticInst->destRegIdx(0), (*it)->staticInst->srcRegIdx(0),(*it)->staticInst->srcRegIdx(1));

        if (position==counter++) {
            if(faultType==2) {
                //~ (*it)->_srcRegIdx[0]=~(regDest ^ faultMask);
                //~ printf("ACHOU:%d\n",(*it)->_srcRegIdx[0]);
            } else if(faultType==3) {
                //~ (*it)->_srcRegIdx[0]=~((*it)->_srcRegIdx[0] ^ faultMask);
            }
            //~ break;
        }
    }
}

/**
 * Context save
 */
void faultInjectionSystem::saveRegisterContext(const std::string file) {
    System * SystemObj = context->getSystemPtr();

    //Get the number of cores (or contexts) available
    int numberOfCores = SystemObj->numContexts();

    //Open temporary file
    ofstream fileptr(file);

    ///Read the integer registers
    for (int i=0;i<numberOfCores;i++)
        for (int faultRegister=0;faultRegister < numberOfIntegerRegisters-1 ;faultRegister++){
            fileptr<<SystemObj->getThreadContext(i)->readIntReg(faultRegister)<<" ";
            //cout<<"Reg #"<<i<<" "<<   <<"\n"
        }

    fileptr<<"\n";

    ///Read the float registers
    for (int i=0;i<numberOfCores;i++)
        for (int faultRegister=0;faultRegister<NumFloatSpecialRegs;faultRegister++)
            fileptr<<SystemObj->getThreadContext(i)->readFloatReg(faultRegister)<<" ";

    fileptr<<"\n";

    ///PC address
    for (int i=0;i<numberOfCores;i++)
        fileptr<<SystemObj->getThreadContext(i)->pcState().instAddr()<<",";

    fileptr<<"\n";

    ///Close the file
    fileptr.close();
}

/**
 * faultInjectionSystem Parameters creator
 */
faultInjectionSystem * faultInjectionSystemParams::create() {
    return new faultInjectionSystem(this);
}
