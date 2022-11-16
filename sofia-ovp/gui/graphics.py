#   Fault Injection Module Graphical Interface
#
#
#
#
#   Author: Felipe Rocha da Rosa

# Libraries
import sys
import os

import subprocess
import linecache
import ctypes
import string
import threading
from array import *
from enum import Enum
from random     import randint
from array      import *
import copy
import re
from operator import itemgetter
from collections import Counter


import matplotlib.pyplot as plt
from itertools import groupby
import pylab as P
import numpy as np
import matplotlib.gridspec as gridspec

# Local classes
from classification import *
from commondefines  import *

class graphics():
    #Copy a list to the DAC classification
    def dacList(self,faultListDAC):
        for fault in faultListDAC:
            if fault[6]==errorAnalysis.Masked_Fault.name:
                fault[6]=errorAnalysisDAC.Vanished.name
                fault[7]=errorAnalysisDAC.Vanished.value

            elif fault[6]==errorAnalysis.Control_Flow_Data_OK.name \
            or fault[6]==errorAnalysis.REG_STATE_Data_OK.name:
                fault[6]=errorAnalysisDAC.Application_Output_Not_Affected.name
                fault[7]=errorAnalysisDAC.Application_Output_Not_Affected.value

            elif fault[6]==errorAnalysis.Control_Flow_Data_ERROR.name \
            or fault[6]==errorAnalysis.REG_STATE_Data_ERROR.name \
            or fault[6]==errorAnalysis.silent_data_corruption.name:
                fault[6]=errorAnalysisDAC.Application_Output_Mismatch.name
                fault[7]=errorAnalysisDAC.Application_Output_Mismatch.value

            elif fault[6]==errorAnalysis.Hang.name:
                fault[6]=errorAnalysisDAC.Hang.name
                fault[7]=errorAnalysisDAC.Hang.value

            else:
                fault[6]=errorAnalysisDAC.Unexpected_Termination.name
                fault[7]=errorAnalysisDAC.Unexpected_Termination.value

    def readReport(self,faultList,fileName):
        simulationTime          = float (subprocess.check_output('grep "Simulation Time (seconds)" %s'%fileName,shell=True,).split()[-1])
        faults                  = int   (subprocess.check_output('grep "Recovered faults" %s'%fileName,shell=True,).split()[-1])
        executedInstructions    = int   (subprocess.check_output('grep "Gold Executed Instructions" %s'%fileName,shell=True,).split()[-1])

        for i in range(2,faults+2):
            faultList.append(linecache.getline(fileName,i).split())

        #Sort the list
        faultList.sort(key=lambda x: int(x[0]))

        return simulationTime,faults,executedInstructions

    def getApplicationName(self,fileName):
        return str(subprocess.check_output('grep "Application" %s'%fileName,shell=True,).split()[-1])

    def getApplicationEnvironment(self,fileName):
        return str(subprocess.check_output('grep "Environment" %s'%fileName,shell=True,).split()[-1])

    def readFolder(self,folderName):
        files=[]
        if self.apps:
            for filename in self.apps.split(","):
                files.append( [ folderName+"/"+folderFile for folderFile in os.listdir(folderName) if filename in folderFile ][0])
        else:
            for filename in os.listdir(folderName):
                if re.match(".*report*", filename) is not None:
                    files.append(folderName+"/"+filename)

        files.sort()
        return files

    def __init__(self,replotfile=None, replotfolder1set=None, replotfolder2set=None, replotfolder3set=None, outputfolder="./", outputfile="plot.eps", apps=None, names=None, environment=None, barnames=False):
        self.replotfile          = replotfile
        self.replotfolder1set    = replotfolder1set
        self.replotfolder2set    = replotfolder2set
        self.replotfolder3set    = replotfolder3set
        self.outputfolder        = outputfolder
        self.outputfile          = outputfile
        self.barnames            = barnames
        self.apps                = apps
        self.names               = names
        self.environment         = environment

        #A,B,C,D...
        self.letterUppercase   = list(string.ascii_uppercase)

        # list of possible registers
        archregisters = archRegisters()

        # registers for each simulator environment
        possibleRegisters = archregisters.possibleregisters(self.environment)
        numberOfRegisters = archregisters.getNumberOfRegisters()

        ####################################################################################################
        # graphics Generation                                                                              #
        ####################################################################################################
        # colours and hatches
        self.colors      = ['dodgerblue', 'lightgrey', 'black', 'darkorange', 'white' , 'yellow' , 'black']
        self.hatches     = [' ', '\\', '+', 'x', '*', 'o', 'O', '.']
        self.lineStyles  = ['-', '--', '-.', ':','-','--']
        self.lineColors  = ['red','green','blue','darkorange','yellow','pink']
        self.lineMarkers = ['o', 'v', '*', 'h', 'd','s','p']

    ####################################################################################################
    # Graphics                                                                                         #
    ####################################################################################################
    '''
    Plot the fault campaing result by classification using the DAC grouping
    '''
    def groupsdac(self):
        # Acquire data
        faultList=[]
        simulationTime, faults, executedInstructions = self.readReport(faultList,self.replotfile)

        faultListDAC = copy.deepcopy(faultList)
        self.dacList(faultListDAC)

        data = list(Counter([ item[7] for item in faultListDAC]).items())
        data.sort()
        x  = [int(item[0]) for item in data]
        y  = [(float(item[1])/faults)*100 for item in data]

        label = [errorAnalysisDAC(int(item[0])).name.replace("_"," ") for item in data]

        x_pos = np.arange(len(x))

        plt.figure(figsize=(14,8))

        #~ for i in range(len(patterns)):
        f = plt.bar(x_pos, y, align='center', alpha=0.5, color=self.colors)

        for i,thisbar in enumerate(f.patches):
        # Set a different hatch for each bar
            thisbar.set_hatch(self.hatches[i%len(self.hatches)])

        plt.ylim([ 0,max(y)*1.10])

        plt.title("Distribution per Grouping")
        plt.ylabel("Faults (%)")
        plt.xticks(x_pos, label, rotation=350, horizontalalignment='left')
        
        figureName="groupsdac"
        plt.show()
        plt.savefig("{0}/{1}.eps".format(self.outputfolder,figureName))
