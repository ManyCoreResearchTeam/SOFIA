#   Fault Injection Module Graphical Interface
#
#   Options and arguments
#
#
#   Author: Felipe Rocha da Rosa

import shlex
from optparse   import OptionParser
from commondefines import *

class optionsFIM():
    def __init__(self,name,buttom=None,buttomobj=None,entry=None,module=None,command=None,action=None,type=None,dest=None,choices=None,default=None,help=None):
        self.name       = name
        self.buttom     = buttom
        self.buttomobj  = buttomobj
        self.entry      = entry
        self.module     = module
        self.command    = command
        self.action     = action
        self.type       = type
        self.dest       = dest
        self.choices    = choices
        self.default    = default
        self.help       = help
        
class parseArgs():
    def __init__(self):
        self.options = []   
        self.options.append(optionsFIM(name="No GUI",              buttom="tick",    buttomobj=None, entry=None, module="UI", command="--nogui",            action="store_true",                    dest="nogui",                                                                       help="UI: Don't start a GUI"))
        self.options.append(optionsFIM(name="Verbose",             buttom="tick",    buttomobj=None, entry=None, module="UI", command="--verbose",          action="store_true",                    dest="verbose",                                                                     help="UI: Display the log's in the terminal"))
        self.options.append(optionsFIM(name="Environment",         buttom="choice",  buttomobj=None, entry=None, module="FI", command="--environment" ,                             type="choice",  dest="environment",     choices=environmentsL,      default=environmentsL[0],       help="Fault campaign: target enviroment"))
        self.options.append(optionsFIM(name="Architecture",        buttom="choice",  buttomobj=None, entry=None, module="FI", command="--architecture",                             type="choice",  dest="architecture",    choices=architecturesL,     default=architecturesL[0],      help="Fault campaign: target architecture"))
        self.options.append(optionsFIM(name="Number of Cores",     buttom="string",  buttomobj=None, entry=None, module="FI", command="--numcores",         action="store",         type="int",     dest="numcores",                                    default="1",                    help="Fault campaign: number of cores in the target architecture"))
        self.options.append(optionsFIM(name="Number of Faults",    buttom="string",  buttomobj=None, entry=None, module="FI", command="--numfaults",        action="store",         type="int",     dest="numfaults",                                   default="1",                    help="Fault campaign: number of faults"))
        self.options.append(optionsFIM(name="Number of Apps",      buttom="string",  buttomobj=None, entry=None, module="FI", command="--numapps",          action="store",         type="int",     dest="numapps",                                     default="1",                    help="Fault campaign: number of applications"))
        self.options.append(optionsFIM(name="Workload",            buttom="choice",  buttomobj=None, entry=None, module="FI", command="--workload",                                 type="choice",  dest="workload",        choices=workloadsL,         default=workloadsL[3],          help="Fault campaign: workload"))
        self.options.append(optionsFIM(name="Apps",                buttom="choice2", buttomobj=None, entry=None, module="FI", command="--apps",             action="store",         type="string",  dest="apps",                                        default="",                     help="Fault campaign: applications"))
        self.options.append(optionsFIM(name="Compilation options", buttom="choice",  buttomobj=None, entry=None, module="FI", command="--compilation",                              type="choice",  dest="compilation",     choices=compilationoptsL,   default=compilationoptsL[0],    help="Fault campaign: "))
        self.options.append(optionsFIM(name="Reg. list",           buttom="string",  buttomobj=None, entry=None, module="FI", command="--registerlist",     action="store",         type="string",  dest="registerlist",                                default="",                     help="Fault generation: list of target register"))
        self.options.append(optionsFIM(name="Fault Type",          buttom="choice",  buttomobj=None, entry=None, module="FI", command="--faulttype",                                type="choice",  dest="faulttype",       choices=faulttypesL,        default=faulttypesL[0],         help="Fault generation: fault type"))
        self.options.append(optionsFIM(name="Dummy Faults",        buttom="tick",    buttomobj=None, entry=None, module="FI", command="--dummy",            action="store_true",                    dest="dummy",                                       default=False,                  help="Fault generation: Create faults that DO NOT generate errors (i.e. any bit will be flipped)"))
        self.options.append(optionsFIM(name="Eq. dist Faults",     buttom="tick",    buttomobj=None, entry=None, module="FI", command="--eqdist",           action="store_true",                    dest="eqdist",                                      default=False,                  help="Fault generation: The number of faults will be evenly distributed among the registers and cores available"))
        self.options.append(optionsFIM(name="Registers File",      buttom="string",  buttomobj=None, entry=None, module="FI", command="--registerfile",     action="store",         type="string",  dest="registerfile",                                default="",                     help="Fault generation: list of target register"))
        self.options.append(optionsFIM(name="memlowaddress",       buttom="string",  buttomobj=None, entry=None, module="FI", command="--memlowaddress",    action="store",         type="long",    dest="memlowaddress",                               default=0x0,                    help="Fault generation: Default base memory address"))
        self.options.append(optionsFIM(name="memhighaddress",      buttom="string",  buttomobj=None, entry=None, module="FI", command="--memhighaddress",   action="store",         type="long",    dest="memhighaddress",                              default=0x8000000,              help="Fault generation: Default base memory size (in hex)"))
        self.options.append(optionsFIM(name="Parallel Cores",      buttom="string",  buttomobj=None, entry=None, module="FI", command="--parallel",         action="store",         type="int",     dest="parallel",                                    default="1",                    help="Parallel: number of cores available to the parallel phases"))
        self.options.append(optionsFIM(name="Trace Symbol",        buttom="string",  buttomobj=None, entry=None, module="TR", command="--tracesymbol",      action="store",         type="string",  dest="tracesymbol",                                 default="",                     help="Trace:"))
        self.options.append(optionsFIM(name="Number of Bit Flips",        buttom="string",  buttomobj=None, entry=None, module="FI", command="--bitflips",      action="store",         type="int",  dest="bitFlips",                                 default=1,                     help="Number of bits to flip"))
        self.options.append(optionsFIM(name="Sequential Bit Flips",        buttom="tick",  buttomobj=None, entry=None, module="FI", command="--seqflip",      action="store_true",           dest="sequentialBits",                                 default=False,                     help="Flip n sequential bits"))
    
    def getOptionL(self):
        return self.options
    
    def parse(self,newargs=None):
        usage = "usage: %prog [options] arg1 arg2"# @Review
        parser = OptionParser(usage=usage)
        
        for item in self.options:
            if item.action=="store":
                parser.add_option(item.command,  action=item.action, type=item.type, dest=item.dest, default=item.default, help=item.help)
            elif item.action=="store_true":
                parser.add_option(item.command,  action=item.action, dest=item.dest, default=item.default, help=item.help)
            else:
                parser.add_option(item.command,  type=item.type, dest=item.dest, choices=item.choices, default=item.default, help=item.help)
    
        # parse the arguments from the terminal
        if newargs==None:
            (options, args) = parser.parse_args()
        else: # parse the arguments from a internal strign
            (options, args) = parser.parse_args(newargs)
        
        return (options, args)
    
