/*
""" 
Fault Injection Module to Gem5
Version 1.0
02/02/2016
    
Author: Felipe Rocha da Rosa - frdarosa.com
"""
 */

#include "params/FaultInjectionSystem.hh"

#include "FIM/FaultInjectionSystem.hh"
using namespace std;

FaultInjectionSystem *fi_system;
/**
 * Class Creator
 */
FaultInjectionSystem::FaultInjectionSystem(const Params *p) 
   : SimObject(p)
   //faultInjectionEvent(this)
{
   ///Cpu type
   CPU_Type=p->CPU_Type;
   
   ///Get the correct context
    if (CPU_Type == 0) {//Base CPU
        ///Cpu object using polymorphism
        core = (BaseCPU*) p->system;
        context =  core->getContext(0);
    } else {///O3 CPU
        coreDerivO3CPU = (DerivO3CPU*) p->system;
        context = coreDerivO3CPU->tcBase(0);
    }
}

/**
 * Fault injector handler
 */
void FaultInjectionSystem::faultInjectionHandler(int faultMask, int faultType, int faultRegister, bool targetPC) {
    //Uns64 tempCount;
    TheISA::IntReg tempRegister;
    TheISA::IntReg tempMask = 0xFFFFFFFF00000000 | faultMask;
    TheISA::IntReg tempResult;
    
    cout<<"FIM: Fault Injection Handler\n";
    printf("PC: 0x%lX MASK: 0x%X REG: %d\n",context->pcState().instAddr(),faultMask,faultRegister);
    
    if (TheISA::inUserMode(context)) {
        printf("IN USER MODE\n");
    }
    
    if(targetPC) {///Program counter fault injection
        ReadPC()
        ApplyMask()
        //~ WritePC()
    } else {///Others registers
        ReadRegister()
        ApplyMask()
        //~ WriteRegister()
    }
    
	printf("Head %d\n",coreDerivO3CPU->rob.numInstsInROB);
	printf("Head %lu\n",coreDerivO3CPU->rob.instList[0].size());
	coreDerivO3CPU->rob.readValue(2,0);
    //~ InstIt RobValue = std::next(rob.instList[0].begin(), N);
	
	//~ cout<<std::next(coreDerivO3CPU->rob.instList[0].begin(), 2)<<"\n"
    
	printf("Before %lX After %lX (%lX) mask %lX\n",tempRegister,tempResult,context->readIntReg(faultRegister),tempMask);
}


/**
 * Context Saver
 */
void FaultInjectionSystem::saveRegisterContext(const std::string file) {
    System * SystemObj = context->getSystemPtr();
    
    //Get the number of cores (or contexts) available
    int NumberOfcores = SystemObj->numContexts();
    
    //Open temporary file
    ofstream fileptr(file);
    
    ///Read the integer registers
    for(int i=0;i<NumberOfcores;i++)
        for(int faultRegister=0;faultRegister<15;faultRegister++)
            fileptr<<SystemObj->getThreadContext(i)->readIntReg(faultRegister)<<" ";
    
    fileptr<<"\n";
    
    ///Read the float registers
    for(int i=0;i<NumberOfcores;i++)
        for(int faultRegister=0;faultRegister<NumFloatSpecialRegs;faultRegister++)
            fileptr<<SystemObj->getThreadContext(i)->readFloatReg(faultRegister)<<" ";
    
    fileptr<<"\n";
    
    ///PC address
    for(int i=0;i<NumberOfcores;i++)
        fileptr<<SystemObj->getThreadContext(i)->pcState().instAddr()<<",";
    
    fileptr<<"\n";
    
    //~ printf("%ld \n",SystemObj->totalNumInsts);
    //~ printf("%ld \n",core->numSimulatedInsts());
        
    ///Close the file
    fileptr.close();
}

/**
 * FaultInjectionSystem Parameters creator
 */
FaultInjectionSystem * FaultInjectionSystemParams::create() {
    return new FaultInjectionSystem(this);
}
