"""
    Fault Injection Module to Gem5
    Version 3.0
    25/02/2017

    Author: Felipe Rocha da Rosa - frdarosa.com
"""

from m5.SimObject import *
from m5.params import *
from m5.proxy import *
from m5.util import fatal
from m5.defines import buildEnv


class faultInjectionSystem(SimObject):
    _the_instance = None

    type = 'faultInjectionSystem'
    cxx_header = "fim/faultInjectionSystem.hh"

    #~ @classmethod
    #~ def export_method_swig_predecls(cls, code):
        #~ code('''%include <std_string.i>''')

    @classmethod
    def export_methods(cls, code):
        code('''
    void faultInjectionHandler();

    void setupFault(int64_t faultMask, int faultType, int faultRegister, bool targetPC,uint64_t memAddress);

    void saveRegisterContext(const std::string file);
    ''')

    mem     = Param.SimpleMemory("Memory obj")
    system  = Param.BaseCPU("CPU running the faults")
    cpuType = Param.Int(0,"CPU type")

