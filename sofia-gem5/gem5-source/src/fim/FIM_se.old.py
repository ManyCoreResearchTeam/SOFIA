""" Fault Injection Module to Gem5
    Version 1.0
    02/02/2016
    
    Author: Felipe Rocha da Rosa - frdarosa.com
"""

import optparse
import sys
import os

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.util import addToPath, fatal

addToPath('../../configs/common')
addToPath('../../configs/ruby')

import Options
import Ruby
import Simulation
import CacheConfig
import CpuConfig
import MemConfig
from Caches import *
from cpu2000 import *

#################### Modified ############################
addToPath('../FIM')
addToPath('./')
from faultInjectionModule import *
#################### Modified ############################

# Check if KVM support has been enabled, we might need to do VM
# configuration if that's the case.
have_kvm_support = 'BaseKvmCPU' in globals()

###############################################################################################################################
###############################################Definition of sub-functions#####################################################
###############################################################################################################################
def is_kvm_cpu(FutureClass):
    return have_kvm_support and FutureClass != None and \
        issubclass(FutureClass, BaseKvmCPU)

def get_processes(options):
    """Interprets provided options and returns a list of processes"""

    multiprocesses = []
    inputs = []
    outputs = []
    errouts = []
    pargs = []

    workloads = options.cmd.split(';')
    if options.input != "":
        inputs = options.input.split(';')
    if options.output != "":
        outputs = options.output.split(';')
    if options.errout != "":
        errouts = options.errout.split(';')
    if options.options != "":
        pargs = options.options.split(';')

    idx = 0
    for wrkld in workloads:
        process = LiveProcess()
        process.executable = wrkld
        process.cwd = os.getcwd()

        if options.env:
            with open(options.env, 'r') as f:
                process.env = [line.rstrip() for line in f]

        if len(pargs) > idx:
            process.cmd = [wrkld] + pargs[idx].split()
        else:
            process.cmd = [wrkld]

        if len(inputs) > idx:
            process.input = inputs[idx]
        if len(outputs) > idx:
            process.output = outputs[idx]
        if len(errouts) > idx:
            process.errout = errouts[idx]

        multiprocesses.append(process)
        idx += 1

    if options.smt:
        assert(options.cpu_type == "detailed")
        return multiprocesses, idx
    else:
        return multiprocesses, 1

def createSystem(options,numThreads,multiprocesses,index=0):
    (CPUClass, test_mem_mode, FutureClass) = Simulation.setCPUClass(options)
    CPUClass.numThreads = numThreads

    #Create the system
    np = options.num_cpus
    system = System(cpu = [CPUClass(cpu_id=i) for i in xrange(np)],
                    mem_mode = test_mem_mode,
                    mem_ranges = [AddrRange(options.mem_size)],
                    cache_line_size = options.cacheline_size)

    if numThreads > 1:
        system.multi_thread = True

    # Create a top-level voltage domain
    system.voltage_domain = VoltageDomain(voltage = options.sys_voltage)

    # Create a source clock for the system and set the clock period
    system.clk_domain = SrcClockDomain(clock =  options.sys_clock,
                                    voltage_domain = system.voltage_domain)

    # Create a CPU voltage domain
    system.cpu_voltage_domain = VoltageDomain()

    # Create a separate clock domain for the CPUs
    system.cpu_clk_domain = SrcClockDomain(clock = options.cpu_clock,
                                        voltage_domain =
                                        system.cpu_voltage_domain)

    # If elastic tracing is enabled, then configure the cpu and attach the elastic
    # trace probe
    if options.elastic_trace_en:
        CpuConfig.config_etrace(CPUClass, system.cpu, options)

    # All cpus belong to a common cpu_clk_domain, therefore running at a common
    # frequency.
    for cpu in system.cpu:
        cpu.clk_domain = system.cpu_clk_domain

    # Sanity check
    if options.fastmem:
        if CPUClass != AtomicSimpleCPU:
            fatal("Fastmem can only be used with atomic CPU!")
        if (options.caches or options.l2cache):
            fatal("You cannot use fastmem in combination with caches!")

    if options.simpoint_profile:
        if not options.fastmem:
            # Atomic CPU checked with fastmem option already
            fatal("SimPoint generation should be done with atomic cpu and fastmem")
        if np > 1:
            fatal("SimPoint generation not supported with more than one CPUs")

    for i in xrange(np):
        if options.smt:
            system.cpu[i].workload = multiprocesses
        elif len(multiprocesses) == 1:
            system.cpu[i].workload = multiprocesses[0]
        else:
            system.cpu[i].workload = multiprocesses[i]

        if options.fastmem:
            system.cpu[i].fastmem = True

        if options.simpoint_profile:
            system.cpu[i].addSimPointProbe(options.simpoint_interval)

        if options.checker:
            system.cpu[i].addCheckerCpu()

        system.cpu[i].createThreads()


    MemClass = Simulation.setMemClass(options)
    system.membus = SystemXBar()
    system.system_port = system.membus.slave
    CacheConfig.config_cache(options, system)
    MemConfig.config_mem(options, system)
    return system,FutureClass

def parsingArguments(FIModule):
    parser = optparse.OptionParser()
    Options.addCommonOptions(parser)
    Options.addSEOptions(parser)
    
    # -------------- Fault Campaign options -------------- #
    FIModule.optionsParser(parser) 
    # ---------------------------------------------------- #

    if '--ruby' in sys.argv:
        Ruby.define_options(parser)

    (options, args) = parser.parse_args()

    if args:
        print "Error: script doesn't take any positional arguments"
        sys.exit(1)

    multiprocesses = []
    numThreads = 1

    if options.bench:
        apps = options.bench.split("-")
        if len(apps) != options.num_cpus:
            print "number of benchmarks not equal to set num_cpus!"
            sys.exit(1)

        for app in apps:
            try:
                if buildEnv['TARGET_ISA'] == 'alpha':
                    exec("workload = %s('alpha', 'tru64', '%s')" % (
                            app, options.spec_input))
                elif buildEnv['TARGET_ISA'] == 'arm':
                    exec("workload = %s('arm_%s', 'linux', '%s')" % (
                            app, options.arm_iset, options.spec_input))
                else:
                    exec("workload = %s(buildEnv['TARGET_ISA', 'linux', '%s')" % (
                            app, options.spec_input))
                multiprocesses.append(workload.makeLiveProcess())
            except:
                print >>sys.stderr, "Unable to find workload for %s: %s" % (
                        buildEnv['TARGET_ISA'], app)
                sys.exit(1)
    elif options.cmd:
        multiprocesses, numThreads = get_processes(options)
    else:
        print >> sys.stderr, "No workload specified. Exiting!\n"
        sys.exit(1)

    return options,numThreads,multiprocesses
###############################################Parsing arguments###############################################################

#Create the Fault injection class
FIModule=FIM()

options,numThreads,multiprocesses = parsingArguments(FIModule)

################################################Create the system###############################################################

system,FutureClass = createSystem(options,numThreads,multiprocesses,0)

################################################Simulation######################################################################

root = Root(full_system = False, system = system)


if options.checkpoint_dir:
    cptdir = options.checkpoint_dir
elif m5.options.outdir:
    cptdir = m5.options.outdir
else:
    cptdir = getcwd()

#Number of cpus in this platform
np = options.num_cpus
switch_cpus = None

if FutureClass:
    switch_cpus = [FutureClass(switched_out=True, cpu_id=(i))
                   for i in xrange(np)]

    for i in xrange(np):
        if options.fast_forward:
            system.cpu[i].max_insts_any_thread = int(options.fast_forward)
        switch_cpus[i].system = system
        switch_cpus[i].workload = system.cpu[i].workload
        switch_cpus[i].clk_domain = system.cpu[i].clk_domain
        switch_cpus[i].progress_interval = \
            system.cpu[i].progress_interval
        # simulation period
        if options.maxinsts:
            switch_cpus[i].max_insts_any_thread = options.maxinsts
        # Add checker cpu if selected
        if options.checker:
            switch_cpus[i].addCheckerCpu()

    # If elastic tracing is enabled attach the elastic trace probe
    # to the switch CPUs
    if options.elastic_trace_en:
        CpuConfig.config_etrace(FutureClass, switch_cpus, options)

    system.switch_cpus = switch_cpus
    switch_cpu_list = [(system.cpu[i], switch_cpus[i]) for i in xrange(np)]

# Handle the max tick settings now that tick frequency was resolved
# during system instantiation
# NOTE: the maxtick variable here is in absolute ticks, so it must
# include any simulated ticks before a checkpoint
explicit_maxticks = 0
maxtick_from_abs = m5.MaxTick
maxtick_from_rel = m5.MaxTick
maxtick_from_maxtime = m5.MaxTick
maxtick = min([maxtick_from_abs, maxtick_from_rel, maxtick_from_maxtime])

################################################Creating the FIModule###########################################################
#Create the fault injection module
FIModule.FIMcreation(options,system,cptdir)

#Call simulation manager - includes the m5.instantiate(checkpoint_dir)
FIModule.simulate(m5)

#After simulation
FIModule.report()

sys.exit()
################################################Creating the FIModule###########################################################
