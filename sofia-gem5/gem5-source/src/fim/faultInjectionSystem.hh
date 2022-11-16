/*
""" 
    Fault Injection Module to Gem5
    Version 3.0
    25/02/2017
    
    Author: Felipe Rocha da Rosa - frdarosa.com
"""
 */

#ifndef __faultInjectionSystem_HH__
#define __faultInjectionSystem_HH__

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
#include "arch/registers.hh"
#include "config/the_isa.hh"

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
#include "params/faultInjectionSystem.hh"

//Gem5 Includes cpus types
#include "cpu/simple/atomic.hh"
#include "cpu/simple/base.hh"
#include "cpu/simple/timing.hh"
#include "cpu/base.hh"
#include "cpu/simple_thread.hh"
#include "cpu/o3/deriv.hh"
#include "cpu/o3/cpu.hh"
#include "cpu/o3/thread_context.hh"

//Gem5 memory
#include "mem/simple_mem.hh"

#define ARMV7_NREGS   16
#define AARCH64_NREGS 33
class faultInjectionSystem : public SimObject
{
  private:
    int  platformIdentifier;

    //CPU context
    BaseCPU * core;
    DerivO3CPU * coreDerivO3CPU;

    SimpleMemory* pmem;

    //Base context to inject the fault
    ThreadContext* context;
    //CPU architecure
    int cpuType;

    // Number of registers to save
    int numberOfIntegerRegisters;

    // Fault Description
    int64_t faultMask;
    int 	faultType;
    int 	faultRegister;
    bool	targetPC;
    uint64_t memAddress;


  public:
    typedef faultInjectionSystemParams Params;
    /**
     * Construct and initialize
     */
    faultInjectionSystem(const Params *p);

    /**
     * Destructor
     */
    ~faultInjectionSystem() {};

    /**
    *
    */
    void setupFault(int64_t faultMask, int faultType, int faultRegister, bool targetPC,uint64_t memAddress);
    /**
    *
    */
    void faultInjectionHandler();

    /**
    *
    */
    template <class O3CPUImpl> 
    void ROBFaultInjector(const int faultMask,const int faultType);

    /**
    *
    */
    void saveRegisterContext(const std::string file);

    /**
    * Create a event handler
    */
    //EventWrapper<faultInjectionSystem, &faultInjectionSystem::faultInjectionHandler> faultInjectionEvent;

};

#endif
