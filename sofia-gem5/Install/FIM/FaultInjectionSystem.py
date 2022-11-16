""" 
Fault Injection Module to Gem5
Version 1.0
02/02/2016
    
Author: Felipe Rocha da Rosa - frdarosa.com
"""

from m5.SimObject import *
from m5.params import *
from m5.proxy import *
from m5.util import fatal
from m5.defines import buildEnv


class FaultInjectionSystem(SimObject):
    _the_instance = None

    type = 'FaultInjectionSystem'
    cxx_header = "FIM/FaultInjectionSystem.hh"

    #~ @classmethod
    #~ def export_method_swig_predecls(cls, code):
        #~ code('''%include <std_string.i>''')

    @classmethod
    def export_methods(cls, code):
        code('''
    void faultInjectionHandler(int faultMask, int faultType, int faultRegister, bool targetPC);
    void saveRegisterContext(const std::string file);
    ''')

    #system = Param.DerivO3CPU("CPU running the faults")
    #system = Param.TimingSimpleCPU("CPU running the faults")
    #system = Param.AtomicSimpleCPU("CPU running the faults")
    system = Param.BaseCPU("CPU running the faults")
    
    
    CPU_Type = Param.Int(0,"CPU running the faults")


