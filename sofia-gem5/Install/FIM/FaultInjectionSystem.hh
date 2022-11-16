/*
""" 
Fault Injection Module to Gem5
Version 1.0
02/02/2016
    
Author: Felipe Rocha da Rosa - frdarosa.com
"""
 */

#ifndef __FaultInjectionSystem_HH__
#define __FaultInjectionSystem_HH__

//c++ Includes
#include <map>
#include <utility> 
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//Arm Arch
#include "arch/arm/registers.hh"

//Gem5 parameters and defines
#include "config/the_isa.hh"
#include "base/types.hh"
#include "base/trace.hh"
#include "base/statistics.hh"
#include "sim/sim_object.hh"
#include "sim/system.hh"
#include "sim/eventq.hh"
#include "sim/sim_events.hh"
#include "sim/serialize.hh"
#include "params/FaultInjectionSystem.hh"

//Gem5 Includes cpus types
#include "cpu/simple/atomic.hh"
#include "cpu/simple/base.hh"
#include "cpu/simple/timing.hh"
#include "cpu/base.hh"
#include "cpu/simple_thread.hh"
#include "cpu/o3/deriv.hh"
#include "cpu/o3/cpu.hh"
#include "cpu/o3/thread_context.hh"

//Macros to inject a fault into integer registers
#define ReadRegister()  tempRegister = context->readIntReg(faultRegister);
#define ApplyMask()     tempResult=~(tempRegister ^ tempMask);
#define WriteRegister() context->setIntReg(faultRegister,tempResult);

//Macros to inject a fault into float registers
#define ReadRegisterFloat()     tempRegisterFloat = context->readFloatReg(faultRegister);
#define WriteRegisterFloat()    context->setFloatReg(tempRegisterFloat,tempRegister);
#define ApplyMaskFloat()        tempRegisterFloat=~(tempRegisterFloat ^ tempMask);

//Macros to inject a fault into the program counter
#define ReadPC()  tempRegister = context->pcState().instAddr();
#define WritePC() context->pcState( (TheISA::PCState) tempResult);
//#define WritePC() TheISA::PCState temps = result; context->pcState(temps);

class FaultInjectionSystem : public SimObject
{
  private:
    int  platformIdentifier;

    
    //CPU context
    BaseCPU * core;
    DerivO3CPU * coreDerivO3CPU;
    
    //Base context to inject the fault
    ThreadContext* context;
    int CPU_Type;
    
  public:
    typedef FaultInjectionSystemParams Params;
    /**
     * Construct and initialize
     */
    FaultInjectionSystem(const Params *p);
    
    /**
     * Destructor
     */
    ~FaultInjectionSystem() {};
    
    /**
    *
    */
    void faultInjectionHandler(int faultMask, int faultType, int faultRegister, bool targetPC);

    /**
    *
    */
    void saveRegisterContext(const std::string file);
    
    /**
    * Create a event handler
    */
    //EventWrapper<FaultInjectionSystem, &FaultInjectionSystem::faultInjectionHandler> faultInjectionEvent;
    
};

#endif
