#!/usr/bin/python

#My imports
import sys
import os

import subprocess
import linecache
import ctypes
import string
from array import *
from enum import Enum
from optparse   import OptionParser
from random     import randint
from array      import *
import copy
import re
from operator import itemgetter
from collections import Counter
#Graphcis
import matplotlib
# Force matplotlib to not use any Xwindows backend.
matplotlib.use('Agg')

import matplotlib.pyplot as plt
from itertools import groupby
import pylab as P
import numpy as np
import matplotlib.gridspec as gridspec

#python common class
from classification import *

#Copy a list to the DAC classification
def dacList(faultListDAC):
    for fault in faultListDAC:
        if fault[6]==errorAnalysis.Masked_Fault.name \
        or fault[6]==errorAnalysis.silent_data_corruption2.name:
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

    faultListDAC = [ fault for fault in faultListDAC if fault[6]!=errorAnalysis.Masked_Fault.name  ]

def readReport(faultList,fileName):
    simulationTime          = float (subprocess.check_output('grep "Simulation Time (seconds)" %s'%fileName,shell=True,).split()[-1])
    faults                  = int   (subprocess.check_output('grep "Recovered faults" %s'%fileName,shell=True,).split()[-1])
    executedInstructions    = int   (subprocess.check_output('grep "Gold Executed Instructions" %s'%fileName,shell=True,).split()[-1])

    for i in range(2,faults+2):
        faultList.append(linecache.getline(fileName,i).split())

    #Sort the list
    faultList.sort(key=lambda x: int(x[0]))

    return simulationTime,faults,executedInstructions

def getApplicationName(fileName):
    return str(subprocess.check_output('grep "Application" %s'%fileName,shell=True,).split()[-1])

def getApplicationEnvironment(fileName):
    return str(subprocess.check_output('grep "Environment" %s'%fileName,shell=True,).split()[-1])

def readFolder(folderName):
    files=[]
    if options.apps:
        for filename in options.apps.split(","):
            files.append( [ folderName+"/"+folderFile for folderFile in os.listdir(folderName) if filename in folderFile ][0])
    else:
        for filename in os.listdir(folderName):
            if re.match(".*report*", filename) is not None:
                files.append(folderName+"/"+filename)

    files.sort()
    return files

# -------------------------------------- Fault Campaign options ------------------------------------- #
usage = "usage: %prog [options] arg1 arg2"# @Review
parser = OptionParser(usage=usage)

#A,B,C,D...
letterUppercase   = list(string.ascii_uppercase)

# @Review
parser.add_option("--FIM_Timing_Trace_file",        action="store", type="string", dest="FIM_Timing_Trace_file", help="File containing the timing stamps from the simulation",   default="./timing.FIM_log")
parser.add_option("--FIM_Memory_Trace_file",        action="store", type="string", dest="FIM_Memory_Trace_file", help="File containing the memory traces from the simulation",   default="./profile.FIM_log")
#~ parser.add_option("--FIM_mips_file",                action="store", type="string", dest="FIM_mips_file",         help="Simulation speed (MIPS) for different simulations modes", default="./mips_info.FIM_log") # @Review

# replot files
parser.add_option("--replotfile",                   action="store", type="string", dest="replotfile",       help="input file", default="./application.reportfile")

# output options
parser.add_option("--replotfolder1set",             action="store", type="string", dest="replotfolder1set", help="...")
parser.add_option("--replotfolder2set",             action="store", type="string", dest="replotfolder2set", help="...")
parser.add_option("--replotfolder3set",             action="store", type="string", dest="replotfolder3set", help="...")
parser.add_option("--replotfolder4set",             action="store", type="string", dest="replotfolder4set", help="...")
parser.add_option("--replotfolder5set",             action="store", type="string", dest="replotfolder5set", help="...")
parser.add_option("--outputfolder",                 action="store", type="string", dest="outputfolder",     help="Plot output folder",default=".")
parser.add_option("--outputfile",                   action="store", type="string", dest="outputfile",       help="Plot output name")

# Table
parser.add_option("--raw",                          action="store_true", dest="raw",                        help="Table")
parser.add_option("--comparison",                   action="store_true", dest="comparison",                 help="Table")
parser.add_option("--detailedcomp",                 action="store_true", dest="detailedcomp",               help="Table")
parser.add_option("--quantum",                      action="store_true", dest="quantum",                    help="Table")

# graphics
parser.add_option("--executedinstructions",         action="store_true", dest="executedinstructions",       help="Graphic - Number of executed instructions by each fault")
parser.add_option("--groups",                       action="store_true", dest="groups",                     help="Graphic - Error analysis per group")
parser.add_option("--groupsdac",                    action="store_true", dest="groupsdac",                  help="Graphic - Error analysis per group using the DAC classification according Cho et al")
parser.add_option("--register",                     action="store_true", dest="register",                   help="Graphic - Inserted fault per register")
parser.add_option("--memorytrace",                  action="store_true", dest="memorytrace",                help="Graphic - Resource utilisation trace")
parser.add_option("--mipsbars",                     action="store_true", dest="mipsbars",                   help="Graphic - ...")
parser.add_option("--scalability",                  action="store_true", dest="scalability",                help="Graphic - ...")
parser.add_option("--speeds",                       action="store_true", dest="speeds",                     help="Graphic - ...")
parser.add_option("--insertiontime",                action="store_true", dest="insertiontime",              help="Graphic - Insertion time for each fault")

parser.add_option("--registers1sets",               action="store_true", dest="registers1sets",             help="Graphic - Error analysis per register using replotfile file1")
parser.add_option("--registers2sets",               action="store_true", dest="registers2sets",             help="Graphic - Error analysis per register for two applications using replotfile file1,file2")
parser.add_option("--registers3sets",               action="store_true", dest="registers3sets",             help="Graphic - Error analysis per register for three applications using replotfile file1,file2,file3")

parser.add_option("--stackedapplications1sets",     action="store_true", dest="stackedapplications1sets",   help="Graphic - Plot a set of applications pointed in replotfolder1set in stacked bars")
parser.add_option("--stackedapplications2sets",     action="store_true", dest="stackedapplications2sets",   help="Graphic - Plot two sets of applications pointed in replotfolder1set and --barCompared2sets in stacked bars")
parser.add_option("--stackedapplications3sets",     action="store_true", dest="stackedapplications3sets",   help="Graphic - Plot three sets of applications pointed in replotfolder1set,replotfolder1set, and barCompared2sets in stacked bars")
parser.add_option("--stackedapplications4sets",     action="store_true", dest="stackedapplications4sets",   help="Graphic - Plot three sets of applications pointed in replotfolder1set,replotfolder1set, and barCompared2sets in stacked bars")
parser.add_option("--stackedapplications5sets",     action="store_true", dest="stackedapplications5sets",   help="Graphic - Plot three sets of applications pointed in replotfolder1set,replotfolder1set, and barCompared2sets in stacked bars")

parser.add_option("--barCompared1sets",             action="store_true", dest="barCompared1sets",           help="Graphic -  - replotfile file1,file2,file3")
parser.add_option("--barCompared2sets",             action="store_true", dest="barCompared2sets",           help="Graphic -  - replotfile file1,file2,file3")
parser.add_option("--barCompared3sets",             action="store_true", dest="barCompared3sets",           help="Graphic -  - replotfile file1,file2,file3")

# grapichs options
parser.add_option("--barnames",                     action="store", type="string", dest="barnames",         help="Optional bar name to the stacked graph", default=",,")
parser.add_option("--appnames",                     action="store", type="string", dest="appnames",         help="Optional name to the stacked graph", default=",,")
parser.add_option("--apps",                         action="store", type="string", dest="apps",             help="Optional applications names (default is all files in the input folder)")
parser.add_option("--names",                        action="store_true", dest="names",                      help="Plot with full name")

# architecture
parser.add_option("--environment",                  type="choice", dest="environment", choices=architectures, help="Choose one architecture mode", default=architectures[0])

# parse the arguments
(options, args) = parser.parse_args()

# list of possible registers
archRegisters = archRegisters()

# registers for each simulator environment
possibleRegisters = archRegisters.getListOfpossibleRegisters(options,parser)
numberOfRegisters = archRegisters.getNumberOfRegisters()

####################################################################################################
# graphics Generation                                                                              #
####################################################################################################
# colours and hatches
colors=['dodgerblue', 'lightgrey', 'black', 'darkorange', 'white' , 'yellow' , 'black']
hatches = [' ', '\\', '+', 'x', '*', 'o', 'O', '.']

lineStyles  = ['-', '--', '-.', ':','-','--']
lineColors  = ['red','green','blue','darkorange','black','pink']
lineMarkers = ['o', 'v', '*', 'h', 'd','s','p']

#~ markers = {'^': 'triangle_up', '<': 'triangle_left', '>': 'triangle_right', '1': 'tri_down', '2': 'tri_up', '3': 'tri_left', '4': 'tri_right', '8': 'octagon', 's': 'square', 'p': 'pentagon', '*': 'star', 'h': 'hexagon1', 'H': 'hexagon2', '+': 'plus', 'x': 'x', 'D': 'diamond', 'd': 'thin_diamond', '|': 'vline', '_': 'hline', 'P': 'plus_filled', 'X': 'x_filled', 0: 'tickleft', 1: 'tickright', 2: 'tickup', 3: 'tickdown', 4: 'caretleft', 5: 'caretright', 6: 'caretup', 7: 'caretdown', 8: 'caretleftbase', 9: 'caretrightbase', 10: 'caretupbase', 11: 'caretdownbase', 'None': 'nothing', None: 'nothing', ' ': 'nothing', '': 'nothing'}
indexPlot=0


####################################################################################################
# Table                                                                                            #
####################################################################################################
# Return raw values
def dumpValues(filelist):
    numberApplicationsPlotted=len(filelist)

    #Array of empty lists
    filesResult= [[] for _ in xrange(numberApplicationsPlotted)]

    #Application Names
    applicationNames=[]

    #Application Names
    applicationEnvironments=[]

    #Read all files and convert
    for i in range(numberApplicationsPlotted):
        readReport(filesResult[i],filelist[i])  # acquire data
        dacList(filesResult[i])                 # convert to DAC classification

        # get the application name from the file
        applicationNames.append(getApplicationName(filelist[i]))
        applicationEnvironments.append(getApplicationEnvironment(filelist[i]))

    # array of empty lists
    data= [[] for _ in xrange(len(errorAnalysisDAC))]

    for i in range(numberApplicationsPlotted):
        for item in filesResult[i]:
            item.append(i)
            data[int(item[7])].append(item)

    # X axis
    x        = np.arange(numberApplicationsPlotted)
    # array used to control each stack height
    yOffset = np.zeros(numberApplicationsPlotted)

    # Print Each line
    for i in range(len(errorAnalysisDAC)):
        if data[i]:
            data[i]=Counter([int(item[12]) for item in data[i]]).items()
            data[i].sort()

            # add missing registers to the list
            a=0
            # collect the faults from the fault list
            for j in range(0,numberApplicationsPlotted):
                if len(data[i])==0:
                    data[i].append((possibleRegisters[j],"0"))
                else:
                    #print j,data[i][a][0]
                    if data[i][a][0] == j:
                        a+=(1 if a<( len(data[i])-1) else 0)
                    else:
                        #~ print "missing",j
                        data[i].append((j,0))
                        a+=1

                data[i].sort()

    return data

# Absolute difference between two raw data
def absDifference(list1,list2,numapps):
    result = []
    for i in range(0,numapps):
        for j in range(len(errorAnalysisDAC)):
            result.append(abs(list1[j][i][1] - list2[j][i][1] ))
    return result

# Return the worst for all classifications
def worstCase(list,numapps):
    result = []
    for i in range(0,numapps):
        base=i*len(errorAnalysisDAC)
        top =base + len(errorAnalysisDAC)
        result.append(max(list[base:top]))
    return result

# Return the total mismatch for application
def totalMismatch(list,numapps):
    result = []
    for i in range(0,numapps):
        base=i*len(errorAnalysisDAC)
        top =base + len(errorAnalysisDAC)
        result.append(sum(list[base:top])*.5)
    return result

# Plot a bar
def plotBar(list,numfiles,barwidth,xoffset,label,color=0):
    global maxyvalue
    x       = np.arange(numfiles)
    y       = [ float( float(item)/8000*100 ) for item in list ]
    yabs    = [ int(item) for item in list ]
    plt.bar(x+xoffset, y, barwidth, align='edge',color=colors[color],hatch=hatches[color],label=label)

    #~ for i in range(numfiles):
        #~ plt.text(i+xoffset+(barwidth/2)-0.05,9,"{0:0.2f}%".format(y[i],yabs[i]),fontsize=11,rotation=90,va="bottom")

    maxyvalue=max(max(y),maxyvalue)

# Dump the raw data in csv
if options.raw:
    # Acquire data
    data1=dumpValues(filelist=readFolder(options.replotfolder1set))
    data2=dumpValues(filelist=readFolder(options.replotfolder2set))
    data3=dumpValues(filelist=readFolder(options.replotfolder3set))
    data4=dumpValues(filelist=readFolder(options.replotfolder4set))

    # Create table with raw data
    table = open("{0}/{1}.csv".format(options.outputfolder,options.outputfile), 'w')
    # Number of application per file (considering a equal number for all folders)
    numfiles = len(readFolder(options.replotfolder1set))
    for j in range(0,numfiles):
        for i in range(len(errorAnalysisDAC)):
            table.write("{0},{1},{2},{3},\n".format(
                        data1[i][j][1],
                        data2[i][j][1],
                        data3[i][j][1],
                        data4[i][j][1]))
    sys.exit()

#Compare 2 groups of data for 1,2, and 4 cores
if options.comparison:
    maxyvalue = 0
    # Reference data
    ref1=dumpValues(filelist=readFolder(options.replotfolder1set.split(',')[0]))
    ref2=dumpValues(filelist=readFolder(options.replotfolder1set.split(',')[1]))
    ref4=dumpValues(filelist=readFolder(options.replotfolder1set.split(',')[2]))
    # Get data for the plot
    data1=dumpValues(filelist=readFolder(options.replotfolder2set.split(',')[0]))
    data2=dumpValues(filelist=readFolder(options.replotfolder2set.split(',')[1]))
    data4=dumpValues(filelist=readFolder(options.replotfolder2set.split(',')[2]))

    # Number of application per file (considering a equal number for all folders)
    numfiles = len(readFolder(options.replotfolder1set.split(',')[0]))
    faultList=[]
    simulationTime,faults,executedInstructions = readReport(faultList,readFolder(options.replotfolder1set.split(',')[0])[0])

	# ONE CLASS Worst Case
    worstcase1 = worstCase(absDifference(ref1,data1,numfiles),numfiles)
    worstcase2 = worstCase(absDifference(ref2,data2,numfiles),numfiles)
    worstcase4 = worstCase(absDifference(ref4,data4,numfiles),numfiles)

    # Absolute Mismatch for all classes
    mismatch1 = totalMismatch(absDifference(ref1,data1,numfiles),numfiles)
    mismatch2 = totalMismatch(absDifference(ref2,data2,numfiles),numfiles)
    mismatch3 = totalMismatch(absDifference(ref4,data4,numfiles),numfiles)
    
    #Info
    print "Absolute Mismatch for all classes"
    print "One core Average {0:.2f}%, Best Case {1:.2f}%, and Worst Case {2:.2f}%".format( (sum(mismatch1)/16/faults*100) ,(min(mismatch1)/faults*100), (max(mismatch1)/faults*100))    
    print "Two cores Average {0:.2f}%, Best Case {1:.2f}%, and Worst Case {2:.2f}%".format( (sum(mismatch2)/16/faults*100) ,(min(mismatch2)/faults*100), (max(mismatch2)/faults*100))    
    print "Four cores Average {0:.2f}%, Best Case {1:.2f}%, and Worst Case {2:.2f}%".format( (sum(mismatch3)/16/faults*100) ,(min(mismatch3)/faults*100), (max(mismatch3)/faults*100))
    
    # Figure
    f = plt.figure(figsize=(14,8))

    #Plot Bar
    barwidth = 0.25
    plotBar(worstcase1,numfiles,barwidth,-0.450,options.barnames.split(',')[0],0)
    plotBar(worstcase2,numfiles,barwidth,-0.125,options.barnames.split(',')[1],1)
    plotBar(worstcase4,numfiles,barwidth,+0.200,options.barnames.split(',')[2],2)

    plt.ylabel("Error Mismatch (%)",fontsize=20)
    plt.ylim([0,maxyvalue+8])
    plt.grid(axis='y')

    plt.xlabel("Applications",fontsize=20)
    plt.xlim(-0.5, float(numfiles)-0.5)
    plt.xticks(np.arange(numfiles),[ letterUppercase[i] for i in range(numfiles)],rotation=0, fontsize=20, horizontalalignment='center')
    plt.yticks(np.arange(0, maxyvalue+1, 5))

    plt.legend(loc='upper right',  shadow=True, ncol=3)
    #Output file
    if options.outputfile:
        figureName=options.outputfile
    else:
        figureName="worstcase"

#One core detailed comparison
if options.detailedcomp:
    maxyvalue = 0
    # Reference data
    ref=dumpValues(filelist=readFolder(options.replotfolder1set))
    # Get data for the plot
    data=dumpValues(filelist=readFolder(options.replotfolder2set))

    # Number of application per file (considering a equal number for all folders)
    numfiles = len(readFolder(options.replotfolder1set))

    # Absolute difference
    absdiff = absDifference(ref,data,numfiles)

    # One list per classification
    eachclassification = []

    # Figure
    f = plt.figure(figsize=(14,8))

    #Plot Bar
    barwidth = 0.15
    for a in range(0,len(errorAnalysisDAC)):
        eachclassification.append([ absdiff[i+a] for i in range(0,len(absdiff)) if (i%len(errorAnalysisDAC))==0 ])
        plotBar(eachclassification[a],numfiles,barwidth,-0.475+((barwidth+.05)*a),errorAnalysisDACShort(a).name,a)

    # Extra text
    #~ plt.text(-.75,9," %       (ABS)",fontsize=11,rotation=90,va="bottom")

    #~ for i in range(0,numfiles):
        #~ plt.axvline(x=i+.5,ymax=9,ls='--')

    plt.ylabel("Error Mismatch (%)",fontsize=20)
    plt.ylim([0,maxyvalue+1])
    plt.grid(axis='y')

    plt.xlabel("Applications",fontsize=20)
    plt.xlim(-0.75, float(numfiles)-0.25)
    plt.xticks(np.arange(numfiles),[ letterUppercase[i] for i in range(numfiles)],rotation=0, fontsize=20, horizontalalignment='center')
    plt.yticks(np.arange(0, maxyvalue+1, 1))

    plt.legend(loc='upper center',  shadow=True, ncol=5)

    #Output file
    if options.outputfile:
        figureName=options.outputfile
    else:
        figureName="worstcasedetailed"

    plt.savefig("{0}/{1}.eps".format(options.outputfolder,figureName))
    sys.exit()

#Quantum difference
if options.quantum:
    maxyvalue = 0

    # Get data for the plot
    data1=dumpValues(filelist=readFolder(options.replotfolder1set))
    data2=dumpValues(filelist=readFolder(options.replotfolder2set))
    data3=dumpValues(filelist=readFolder(options.replotfolder3set))
    data4=dumpValues(filelist=readFolder(options.replotfolder4set))
    ref=  dumpValues(filelist=readFolder(options.replotfolder5set))

    # Number of application per file (considering a equal number for all folders)
    numfiles = len(readFolder(options.replotfolder1set.split(',')[0]))
    faultList=[]
    simulationTime,faults,executedInstructions = readReport(faultList,readFolder(options.replotfolder1set.split(',')[0])[0])
  
    # Absolute mismatch
    print absDifference(ref,data1,numfiles)
    mismatch1 = totalMismatch(absDifference(ref,data1,numfiles),numfiles)
    mismatch2 = totalMismatch(absDifference(ref,data2,numfiles),numfiles)
    mismatch3 = totalMismatch(absDifference(ref,data3,numfiles),numfiles)
    mismatch4 = totalMismatch(absDifference(ref,data4,numfiles),numfiles)

    #Info
    print "Absolute Mismatch for all classes"
    print "Average {0:.2f}%, Best Case {1:.2f}%, and Worst Case {2:.2f}%".format( (sum(mismatch1)/16/faults*100) ,(min(mismatch1)/faults*100), (max(mismatch1)/faults*100))


    # Figure
    f = plt.figure(figsize=(14,8))

    #Plot Bar
    barwidth = 0.20
    plotBar(mismatch1,numfiles,barwidth,-0.402,options.barnames.split(',')[0],0)
    plotBar(mismatch2,numfiles,barwidth,-0.201,options.barnames.split(',')[1],1)
    plotBar(mismatch3,numfiles,barwidth,+0.001,options.barnames.split(',')[2],2)
    plotBar(mismatch4,numfiles,barwidth,+0.202,options.barnames.split(',')[3],3)

    plt.ylabel("Mismatch (%)",fontsize=20)
    plt.ylim([0,maxyvalue+2])
    plt.grid(axis='y')

    plt.xlabel("Applications",fontsize=20)
    plt.xlim(-0.5, float(numfiles)-0.5)
    plt.xticks(np.arange(numfiles),[ letterUppercase[i] for i in range(numfiles)],rotation=0, fontsize=20, horizontalalignment='center')
    plt.yticks(np.arange(0,50, 5))

    plt.legend(loc='upper right',  shadow=True, ncol=5)
    #Output file
    if options.outputfile:
        figureName=options.outputfile
    else:
        figureName="quantumbars"
####################################################################################################
# Graphics                                                                                         #
####################################################################################################
'''
Plot the number of instructions executed for all simulated application
'''
if options.executedinstructions:
    # acquire data
    faultList=[]
    simulationTime,faults,executedInstructions= readReport(faultList,options.replotfile)
    data = Counter([ int(item[9]) for item in faultList]).items()
    data.sort()

    y    = [ (float(item[1])/faults)*100 for item in data]
    x    = [ item[0] for item in data]

    # plot
    plt.figure(figsize=(14,8))
    plt.scatter(x,y)
    plt.bar(x,y,color='dodgerblue')

    plt.title("Instruction Count per Run")
    plt.xlabel("Executed Instructions")
    plt.ylabel("Fault Injections (%)")
    plt.grid(True)
    plt.ylim([0,max(y)])
    plt.xlim([ float(min(x))*0.95,int(executedInstructions)*1.05])
    plt.axvline(int(executedInstructions),color='k', linestyle='--')
    plt.text(int(executedInstructions),max(y)/2,'gold instruction count',rotation='vertical',verticalalignment='center', horizontalalignment='right')

    figureName="executedinstructions"

'''
Plot the fault campaing result by classification using all groups
'''
if options.groups:
    #Acquire data
    faultList=[]
    simulationTime,faults,executedInstructions= readReport(faultList,options.replotfile)
    data = Counter([ item[7] for item in faultList]).items()
    data.sort()
    x  = [int(item[0]) for item in data]
    y  = [(float(item[1])/faults)*100 for item in data]

    label = [errorAnalysis(int(item[0])).name.replace("_"," ") for item in data]

    x_pos = np.arange(len(x))

    plt.figure(figsize=(14,8))
    f = plt.bar(x_pos, y, align='center', alpha=0.5, color=colors)

    for i,thisbar in enumerate(f.patches):
    # Set a different hatch for each bar
        thisbar.set_hatch(hatches[i%len(hatches)])


    plt.ylim([ 0,max(y)*1.10])
    plt.title("Distribution per Grouping")
    plt.ylabel("Faults (%)")
    plt.xticks(x_pos, label, rotation=350, horizontalalignment='left')
    figureName="groups"

'''
Plot the fault campaing result by classification using the DAC grouping
'''
if options.groupsdac:
    #Acquire data
    faultList=[]
    simulationTime,faults,executedInstructions= readReport(faultList,options.replotfile)

    faultListDAC = copy.deepcopy(faultList)
    dacList(faultListDAC)

    data = Counter([ item[7] for item in faultListDAC]).items()
    data.sort()
    x  = [int(item[0]) for item in data]
    y  = [(float(item[1])/faults)*100 for item in data]

    label = [errorAnalysisDAC(int(item[0])).name.replace("_"," ") for item in data]

    x_pos = np.arange(len(x))

    plt.figure(figsize=(14,8))

    #~ for i in range(len(patterns)):
    f = plt.bar(x_pos, y, align='center', alpha=0.5, color=colors)

    for i,thisbar in enumerate(f.patches):
    # Set a different hatch for each bar
        thisbar.set_hatch(hatches[i%len(hatches)])

    plt.ylim([ 0,max(y)*1.10])

    plt.title("Distribution per Grouping")
    plt.ylabel("Faults (%)")
    plt.xticks(x_pos, label, rotation=350, horizontalalignment='left')
    figureName="groupsdac"

'''
Plot the number of injected fault per register WITHOUT any grouping
'''
if options.register:
    # acquire data
    faultList=[]
    simulationTime,faults,executedInstructions= readReport(faultList,options.replotfile)
    data = Counter([ item[2] for item in faultList]).items()
    data.sort()

    y  = [int(item[1]) for item in data]
    x  = [(item[0]) for item in data]

    f = plt.figure(figsize=(14,8))
    x_pos=np.arange(len(x))

    plt.bar(x_pos, y, align='center', alpha=0.5, color=colors)
    plt.ylim([0,max(y)*1.25])
    plt.xlim([-1,len(x)])

    plt.title("Distribution per Register")
    plt.xlabel("Registers")
    plt.ylabel("Injected Faults")
    plt.xticks(x_pos, x, rotation=0, horizontalalignment='center')
    figureName="register"

'''
Plot the number injected faults versus insertion time
'''
if options.insertiontime:
    #Acquire data
    faultList=[]
    simulationTime,faults,executedInstructions= readReport(faultList,options.replotfile)
    data = Counter([ int(item[4]) for item in faultList]).items()
    data.sort()

    y    = [ int(item[1])for item in data]
    x    = [ item[0] for item in data]

    #plot
    f = plt.figure(figsize=(14,8))

    plt.scatter(x,y,color='black')
    plt.bar(x,y,color='lime',edgecolor = "none")

    plt.grid(True)
    plt.ylim([0,max(y)])
    plt.xlim([0,int(executedInstructions)])
    plt.yticks(np.arange(max(y)+1))

    plt.title("Fault Insertion Time")
    plt.xlabel("Number of instructions (Time)")
    plt.ylabel("Number of injected faults")
    figureName="insertiontime"

####################################################################################################
# Bar Stacked per registers                                                                        #
####################################################################################################
'''
These three plots display one, two or three applications showing each register as a stacked bar.
The comparison can be done only during a replot, where the FIM_replot_file options define one to three
file separated by comma.
'''
def plotBarStacked(barWidth,faultList,offset=0,label=False,mode=None):
    global indexPlot
    global maxYaxisValue

    #Convert the list grouping
    faultListDAC = copy.deepcopy(faultList)
    dacList(faultListDAC)

    #Array of empty lists
    data= [[] for _ in xrange(len(errorAnalysisDAC))]

    #Add the dat
    for item in faultListDAC:
        data[int(item[7])].append(item)

    x =  np.arange(numberOfRegisters)

    #Array used to control each stack height
    yOffset = np.zeros(numberOfRegisters)

    for i in range(len(errorAnalysisDAC)):
        data[i] = Counter([ int(item[3]) for item in data[i]]).items()
        data[i].sort()

        #Add missing registers
        a=0
        #Collect the faults from the fault list
        for j in range(0,numberOfRegisters):
            if len(data[i])==0:
                data[i].append((possibleRegisters[j],"0"))
            else:
                #print j,data[i][a][0]
                if data[i][a][0] == j:
                    a+=(1 if a<( len(data[i])-1) else 0)
                else:
                    #print "missing",j
                    data[i].append((j,0))
                    a+=1

            data[i].sort()

        y  = [ (float(item[1])/len(faultListDAC))*100 for item in data[i]]
        if label:
            plt.bar(x+offset, y, barWidth, bottom=yOffset, align='edge',color=colors[i],hatch=hatches[i], label=errorAnalysisDAC(i).name.replace("_", " "))
        else:
            plt.bar(x+offset, y, barWidth, bottom=yOffset, align='edge',color=colors[i],hatch=hatches[i])
        yOffset = yOffset + y

    # Letter or symbol at the top of the bar
    if(mode!=None):
        for x in range(numberOfRegisters):
            plt.text(x+offset+(barWidth/2)-0.05,max(yOffset)+0.1,mode,fontsize=12)

    maxYaxisValue=max(max(yOffset),maxYaxisValue)
    indexPlot +=1

if options.registers1sets:
    maxYaxisValue=0
    indexPlot=0

    faultList=[]
    simulationTime,faults,executedInstructions= readReport(faultList,options.replotfile)
    f = plt.figure(figsize=(14,8))
    plotBarStacked(barWidth =.5,offset=-.250, faultList=faultList,label=True)

    plt.title ("Distribution per Register")
    plt.xlabel("Registers")
    plt.ylabel("Faults (%)")
    plt.ylim([0,maxYaxisValue+10])
    plt.grid(axis='y')
    plt.xlim([-1,numberOfRegisters])
    plt.xticks(np.arange(numberOfRegisters),[ possibleRegisters[i] for i in range(numberOfRegisters)], rotation=0, horizontalalignment='center')
    plt.legend(loc='upper center', shadow=True)
    figureName=("registers2sets")

if options.registers2sets:
    maxYaxisValue=0
    indexPlot=0
    # split files
    files=options.replotfile.split(",")

    faultList=[]
    readReport(faultList,files[0]) #Acquire data
    f = plt.figure(figsize=(14,8))
    plotBarStacked(barWidth =-0.35,offset=-0.05, faultList=faultList,label=True)

    faultList=[]
    readReport(faultList,files[1]) #Acquire data
    plotBarStacked(barWidth = 0.35,offset=0.05,faultList=faultList)

    plt.title("Distribution per Register")
    plt.xlabel("Registers")
    plt.ylabel("Faults (%)")
    plt.ylim([0,maxYaxisValue+10])
    plt.grid(axis='y')
    plt.xlim([-1,numberOfRegisters])
    plt.xticks(np.arange(numberOfRegisters),[ possibleRegisters[i] for i in range(numberOfRegisters)], rotation=0, horizontalalignment='center')
    plt.legend(loc='upper center', shadow=True, ncol=2)
    figureName="registers2sets"

if options.registers3sets:
    maxYaxisValue=0
    indexPlot=0
    f = plt.figure(figsize=(14,8))

    files=options.replotfile.split(",")

    faultList=[]
    readReport(faultList,files[0]) # acquire data
    plotBarStacked(barWidth =-0.20,offset=-0.15, faultList=faultList,label=True)

    faultList=[]
    readReport(faultList,files[1]) # acquire data
    plotBarStacked(barWidth = 0.20,offset=-0.10,faultList=faultList)

    faultList=[]
    readReport(faultList,files[2]) # acquire data
    plotBarStacked(barWidth = 0.20,offset=0.15,faultList=faultList)

    plt.title("Distribution per Register")
    plt.xlabel("Registers")
    plt.ylabel("Faults (%)")
    plt.ylim([0,maxYaxisValue+10])
    plt.grid(axis='y')
    plt.xlim([-1,numberOfRegisters])
    plt.xticks(np.arange(numberOfRegisters),[ possibleRegisters[i] for i in range(numberOfRegisters)], rotation=0, horizontalalignment='center')
    plt.legend(loc='upper center', shadow=True, ncol=3)

    figureName="registers3sets"

####################################################################################################
# Bar Stacked multiple application                                                                 #
# Multiple applications plot side by side                                                          #
####################################################################################################
"""
Create a stacked bar with offset from the middle from a file list
"""
def plotBarStackedApplication(barWidth,filelist,offset=0,label=False,mode=None,appnames=None):
    global maxYaxisValue
    global indexPlot
    global numberApplicationsPlotted

    #print filelist
    numberApplicationsPlotted=len(filelist)

    #Array of empty lists
    filesResult= [[] for _ in xrange(numberApplicationsPlotted)]

    #Application Names
    applicationNames=[]

    #Application Names
    applicationEnvironments=[]

    # Read all files and convert
    for i in range(numberApplicationsPlotted):
        readReport(filesResult[i],filelist[i])  # acquire data
        dacList(filesResult[i])                 # convert to DAC classification

        # get the application name from the file
        applicationNames.append(getApplicationName(filelist[i]))
        applicationEnvironments.append(getApplicationEnvironment(filelist[i]))

    # Custom names
    if appnames != None:
        applicationNames = appnames

    # array of empty lists
    data= [[] for _ in xrange(len(errorAnalysisDAC))]

    # Collect the results into the data list
    for i in range(numberApplicationsPlotted):
        for item in filesResult[i]:
            item.append(i)
            data[int(item[7])].append(item)

    # X axis
    x        = np.arange(numberApplicationsPlotted)
    # array used to control each stack height
    yOffset = np.zeros(numberApplicationsPlotted)

    # Plot and label
    for i in range(len(errorAnalysisDAC)):
        # Include missing class to force the label out
        if data[i]:
            data[i]=Counter([int(item[12]) for item in data[i]]).items()
            data[i].sort()

            # add missing items to the list
            a=0
            # collect the faults from the fault list
            for j in range(0,numberApplicationsPlotted):
                if len(data[i])==0:
                    data[i].append((possibleRegisters[j],"0"))
                else:
                    #print j,data[i][a][0]
                    if data[i][a][0] == j:
                        a+=(1 if a<( len(data[i])-1) else 0)
                    else:
                        #~ print "missing",j
                        data[i].append((j,0))
                        a+=1

                data[i].sort()
        else:
            data[i] = [ (j,0) for j in range(numberApplicationsPlotted)]

        y  = [ (float(item[1])/len(filesResult[item[0]]))*100 for item in data[i]]
        # print each group % value
        #~ print y
        if label:
            plt.bar(x+offset, y, barWidth, bottom=yOffset, align='edge',color=colors[i],hatch=hatches[i], label=errorAnalysisDAC(i).name.replace("_", " "))
            if options.names:
                plt.xticks(np.arange(numberApplicationsPlotted),[ applicationNames[i] for i in range(numberApplicationsPlotted)],rotation=0, fontsize=20, horizontalalignment='center')
            else:
                plt.xticks(np.arange(numberApplicationsPlotted),[ letterUppercase[i] for i in range(numberApplicationsPlotted)],rotation=0, fontsize=20, horizontalalignment='center')

            plt.xlim([-1,numberApplicationsPlotted])
            plt.yticks(fontsize=20)
        else:
            plt.bar(x+offset, y, barWidth, bottom=yOffset, align='edge',color=colors[i],hatch=hatches[i])

        yOffset = yOffset + y


    # plot a letter on top of the bars
    if(mode!=None):
        #~ for x in range(numberApplicationsPlotted):
        for x in range(1):
            plt.text(x+offset+(barWidth/2)-0.100,max(yOffset)+2,mode,fontsize=14,rotation=90,va="bottom")

    # the highest value plotted in the Y-axis
    maxYaxisValue=max(max(yOffset),maxYaxisValue)

    # index to count the number of groups
    indexPlot +=1

# use for a groups of applications
if options.stackedapplications1sets or options.stackedapplications2sets or options.stackedapplications3sets or options.stackedapplications4sets or options.stackedapplications5sets:
    #The highest value Plotted in each axis
    maxYaxisValue = 0
    #Index to count the number of groups
    indexPlot     = 0
    #Nummber of columns
    numberApplicationsPlotted = 0

    f = plt.figure(figsize=(14,8))

    if   options.stackedapplications1sets:
        figureName="stackedapplications1sets"
        plotBarStackedApplication(barWidth =.5,   offset=-.25,  filelist=readFolder(options.replotfolder1set),label=True,appnames=options.appnames.split(','))
    elif options.stackedapplications2sets:
        figureName="stackedapplications2sets"
        plotBarStackedApplication(barWidth =-.35, offset=-.05,  filelist=readFolder(options.replotfolder1set),label=True, mode=options.barnames.split(',')[0],appnames=options.appnames.split(','))
        plotBarStackedApplication(barWidth =.35,  offset=.05,   filelist=readFolder(options.replotfolder2set),label=False,mode=options.barnames.split(',')[1])
    elif options.stackedapplications3sets:
        figureName="stackedapplications3sets"
        plotBarStackedApplication(barWidth =-.2,  offset=-.15,  filelist=readFolder(options.replotfolder1set),label=True, mode=options.barnames.split(',')[0],appnames=options.appnames.split(','))
        plotBarStackedApplication(barWidth =.2,   offset=-.10,  filelist=readFolder(options.replotfolder2set),label=False,mode=options.barnames.split(',')[1])
        plotBarStackedApplication(barWidth =.2,   offset=.15,   filelist=readFolder(options.replotfolder3set),label=False,mode=options.barnames.split(',')[2])
    elif options.stackedapplications4sets:
        figureName="stackedapplications4sets"
        plotBarStackedApplication(barWidth =.15,   offset=-.35-0.025, filelist=readFolder(options.replotfolder1set),label=True, mode=options.barnames.split(',')[0],appnames=options.appnames.split(','))
        plotBarStackedApplication(barWidth =.15,   offset=-.15-0.025, filelist=readFolder(options.replotfolder2set),label=False,mode=options.barnames.split(',')[1])
        plotBarStackedApplication(barWidth =.15,   offset=0.05-0.025, filelist=readFolder(options.replotfolder3set),label=False,mode=options.barnames.split(',')[2])
        plotBarStackedApplication(barWidth =.15,   offset=0.25-0.025, filelist=readFolder(options.replotfolder4set),label=False,mode=options.barnames.split(',')[3])
    elif options.stackedapplications5sets:
        figureName="stackedapplications5sets"
        plotBarStackedApplication(barWidth =.15,   offset=-.395, filelist=readFolder(options.replotfolder1set),label=True, mode=options.barnames.split(',')[0],appnames=options.appnames.split(','))
        plotBarStackedApplication(barWidth =.15,   offset=-.235, filelist=readFolder(options.replotfolder2set),label=False,mode=options.barnames.split(',')[1])
        plotBarStackedApplication(barWidth =.15,   offset=-.075, filelist=readFolder(options.replotfolder3set),label=False,mode=options.barnames.split(',')[2])
        plotBarStackedApplication(barWidth =.15,   offset=0.085, filelist=readFolder(options.replotfolder4set),label=False,mode=options.barnames.split(',')[3])
        plotBarStackedApplication(barWidth =.15,   offset=0.245, filelist=readFolder(options.replotfolder5set),label=False,mode=options.barnames.split(',')[4])

    #~ plt.title("Error Analysis",fontsize=20)
    ybottom=0
    plt.xlabel("Applications",fontsize=20)
    plt.ylabel("Injected Faults (%)",fontsize=20)
    plt.ylim([ybottom,maxYaxisValue+(maxYaxisValue-ybottom)*0.3])
    plt.xlim(-0.5, float(numberApplicationsPlotted)-0.5)
    plt.yticks(np.arange(ybottom, 100.1, 10))
    plt.grid(axis='y')

    plt.legend(loc='upper right',  shadow=True, ncol=2)

####################################################################################################
# Bar Stacked per registers                                                                        #
####################################################################################################
'''
...........
'''
def plotBarCompared(barWidth,faultList,ax,offset=0,label=None,mode=None):
    global indexPlot
    global maxYaxisValue
    global storedYaxis

    # convert the list grouping
    faultListDAC = copy.deepcopy(faultList)
    dacList(faultListDAC)

    # X-axis
    xlength = len(errorAnalysisDAC)
    x = np.arange(xlength)

    # array used to control each stack height
    yOffset = np.zeros(xlength)

    # get the number of occurrences for each class
    y = Counter([ item[7] for item in faultListDAC]).items()

    for i in range(xlength):
        if y[i][0]!=i:
            y.append((i,1))
            y.sort()

    yOffset = [ item[1] for item in y]
    i=indexPlot
    #~ print y


    if label:
        ax.bar(x+offset, yOffset, barWidth, align='edge',color=colors[i],hatch=hatches[i], label=label)
    else:
        ax.bar(x+offset, yOffset, barWidth, align='edge',color=colors[i],hatch=hatches[i])


    # letter or symbol at the top of the bar
    if(mode!=None):
        for x in range(xlength):
            ax.text(x+offset+(barWidth/2)-0.05,max(yOffset)+0.1,mode,fontsize=12)

    storedYaxis.append(yOffset)
    maxYaxisValue=max(max(yOffset),maxYaxisValue)
    indexPlot +=1

if options.barCompared1sets: # Not in use
    sys.exit()

if options.barCompared2sets:
    maxYaxisValue= 0
    indexPlot    = 0
    storedYaxis  = []

    gs = gridspec.GridSpec(2, 1, height_ratios=[1, 3])
    gs.update(left=0.10, right=0.90, hspace=0.00) # lateral spacing
    ax1 = plt.subplot(gs[0])
    ax2 = plt.subplot(gs[1])

    files=options.replotfile.split(",")

    faultList=[]
    simulationTime,faults,executedInstructions=readReport(faultList,files[0]) # acquire data
    plotBarCompared(barWidth =-0.35,offset=-0.05, faultList=faultList,label="Atomic",ax=ax2)

    faultList=[]
    readReport(faultList,files[1]) # acquire data
    plotBarCompared(barWidth = 0.35,offset=0.05,faultList=faultList,label="OVP",ax=ax2)

    xlength = len(errorAnalysisDAC)

    y2 = [(abs(float(storedYaxis[1][i])/float(storedYaxis[0][i]) -1)) *100 for i in range(xlength)]

    # top plot
    ax1.plot(range(xlength), y2,linestyle='--',label="OVPvsAtomic")

    # top figure
    applicationName=getApplicationName(files[1])
    ax1.set_title(applicationName)
    ax1.axis(ymin=0,ymax=max(y2)+10,xmin=-1,xmax=xlength)
    ax1.yaxis.tick_right()
    ax1.yaxis.set_label_position("right")
    ax1.set_xticks([])
    ax1.grid(axis="y",linewidth=.5)
    ax1.set_ylabel("Error (%)")
    ax1.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc=3, ncol=2, mode="expand", borderaxespad=0.,fontsize=10).draw_frame(False)

    # include error text
    for i in range(xlength):
        ax1.text(i,y2[i], str("%.2f" % y2[i]) ,fontsize=12,horizontalalignment='center')

    plt.xlabel("Classification")
    plt.ylabel("Injected Faults")
    plt.ylim([0,maxYaxisValue*1.10])
    plt.grid(axis='y')
    plt.xlim([-1,xlength])
    plt.xticks(np.arange(xlength),[ errorAnalysisDACShort(i).name.replace("_", " ") for i in range(xlength)], rotation=0, horizontalalignment='center')
    plt.legend(loc='upper right', shadow=True)

    figureName="{0}.barCompared2sets".format(applicationName)

if options.barCompared3sets:
    maxYaxisValue= 0
    indexPlot    = 0
    storedYaxis  = []

    gs = gridspec.GridSpec(2, 1, height_ratios=[1, 3])
    gs.update(left=0.10, right=0.90, hspace=0.00) # lateral spacing
    ax1 = plt.subplot(gs[0])
    ax2 = plt.subplot(gs[1])

    files=options.replotfile.split(",")

    faultList=[]
    simulationTime,faults,executedInstructions= readReport(faultList,files[0]) # acquire data
    plotBarCompared(barWidth =-0.20,offset=-0.15, faultList=faultList,  label=options.barnames.split(',')[0],ax=ax2)

    faultList=[]
    readReport(faultList,files[1]) # acquire data
    plotBarCompared(barWidth = 0.20,offset=-0.10,faultList=faultList,   label=options.barnames.split(',')[1],ax=ax2)

    faultList=[]
    readReport(faultList,files[2]) # acquire data
    plotBarCompared(barWidth = 0.20,offset=0.15,faultList=faultList,    label=options.barnames.split(',')[2],ax=ax2)

    xlength = len(errorAnalysisDAC)

    y2 = [(abs(float(storedYaxis[0][i])/float(storedYaxis[2][i]) -1)) *100 for i in range(xlength)]
    y3 = [(abs(float(storedYaxis[1][i])/float(storedYaxis[2][i]) -1)) *100 for i in range(xlength)]

    y2 = [(abs( float(storedYaxis[1][i]-storedYaxis[0][i])/float(faults))) *100 for i in range(xlength)]
    y3 = [(abs( float(storedYaxis[2][i]-storedYaxis[0][i])/float(faults))) *100 for i in range(xlength)]

    #~ print y2
    #~ print [(storedYaxis[0][i]-storedYaxis[2][i]) for i in range(xlength)]
    #~ print y3
    #~ print [(storedYaxis[1][i]-storedYaxis[2][i]) for i in range(xlength)]

    # top plot
    ax1.plot([ i-.25 for i in range(xlength)], y2,marker='o',linestyle='--',label=options.barnames.split(',')[3])
    ax1.plot([ i+.25 for i in range(xlength)], y3,marker='v',linestyle=':', label=options.barnames.split(',')[4])

    # top figure
    applicationName=getApplicationName(files[2])
    ax1.set_title(applicationName)
    ax1.axis(ymin=0,ymax=max(max(y2),max(y3))+10,xmin=-1,xmax=xlength)
    ax1.yaxis.tick_right()
    ax1.yaxis.set_label_position("right")
    ax1.set_xticks([])
    ax1.grid(axis="y",linewidth=.5)
    ax1.set_ylabel("Mismatch (%)")
    ax1.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc=3, ncol=2, mode="expand", borderaxespad=0.,fontsize=14).draw_frame(False)

    # include the error labels
    for i in range(xlength):
        ax1.text(i-.25,y2[i]+2, str("%.2f" % y2[i]) ,fontsize=14,horizontalalignment='center')
        ax1.text(i+.25,y3[i]+2, str("%.2f" % y3[i]) ,fontsize=14,horizontalalignment='center')

    plt.xlabel("Classification")
    plt.ylabel("Injected Faults")
    plt.ylim([0,maxYaxisValue*1.10])
    plt.grid(axis='y')
    plt.xlim([-1,xlength])
    plt.xticks(np.arange(xlength),[ errorAnalysisDACShort(i).name.replace("_", " ") for i in range(xlength)], rotation=0, horizontalalignment='center')
    plt.legend(loc='upper right', shadow=True)

    figureName="{0}.barCompared3sets".format(applicationName)

####################################################################################################
# Memory Trace                                                                                     #
####################################################################################################
if options.memorytrace:
    #Number of processors
    nproc=int(subprocess.check_output('nproc',shell=True,))*100

    data=[]
    dataXtics=[]
    with open(options.FIM_Memory_Trace_file, 'r') as f:
        for row in f:
            data.append(row.split())

    x1 = [int(item[0]) for item in data]
    x2 = [float(item[1]) for item in data]

    with open(options.FIM_Timing_Trace_file, 'r') as f:
        for row in f:
            dataXtics.append(row.split())

    totalTime=int(dataXtics[len(dataXtics)-1][1])
    points = (float(totalTime)/float(len(x1)-1))

    y  = np.linspace(0,totalTime,len(x1))

    #print len(y)
    #print len(x1)

    fig, ax1 = plt.subplots(figsize=(14,8))
    ax1.plot(y,x1,'b-')
    ax1.set_xlabel('time (s)')
    ax1.set_ylabel('Memory usage (Kilobytes)', color='b')

    ax2 = ax1.twinx()
    ax2.plot(y,x2, 'r.')
    ax2.set_ylabel('processor usage (%)', color='r')

    plt.xlim([ 0,totalTime])

    xticks = np.arange(0,totalTime, totalTime/10)
    plt.xticks(xticks)

    [plt.axvline(item[1], linewidth=1, color='g',linestyle='--') for item in dataXtics]
    [plt.text(item[1],max(x2)*0.15,item[0].replace("_", " "),rotation='vertical',verticalalignment='center', horizontalalignment='right') for item in dataXtics]

    plt.title("Fault Injection resource allocation")
    plt.xlabel("Time")

    figureName="timing"

####################################################################################################
# MIPS                                                                                             #
####################################################################################################
if options.mipsbars:
    mipsList=[]
    barWidth = 1.0
    yMax=0

    barWidths  = [- .2, .2, .2]
    barOffsets = [-.15,-.1,.15]

    f = plt.figure(figsize=(14,8))
    files = options.replotfile.split(",")

    for i in range(len(files)):
        # Read file input
        for line in open(files[i], 'r'):
            mipsList.append(line.split())

        numberOfCores=len(mipsList[0])-1

        app=copy.deepcopy(mipsList[i])
        appName=app[0]
        app.pop(0)

        for j in range(numberOfCores):


            yMax=max(yMax,float(app[j]))
            plt.bar(j+barOffsets, app, barWidths[i],align='edge',color=colors[i],hatch=hatches[i])
        #~ plt.bar(i,float(mipsList[i][0]), barWidth, align='edge',color=colors[int(mipsList[i][2])],hatch=hatches[int(mipsList[i][2])], label=mipsList[i][1].replace("_", " "))

    plt.xlim(0,i)
    plt.ylim(0,y_max*1.25)

    plt.xticks([3,14],["Atomic","Detailed"],fontsize=20)
    plt.ylabel("Faults (%)",fontsize=20)
    plt.legend(loc='upper center', shadow=True, ncol=2)
    figureName="scalabilty"

if options.speeds:
    barWidths  = [- .2, .2, .2]
    barOffsets = [-.15,-.1,.15]
    yMax=0.0

    f = plt.figure(figsize=(10,12))
    files = options.replotfile.split(",")

    plt.yscale('log')

    for j in range(len(files)):
        # Read file input
        mipsList=[]
        for line in open(files[j], 'r'):
            mipsList.append(line.split())

        numberOfCores=len(mipsList[0])-1
        rangeToPlot=range(1,numberOfCores+1)

        #plot data
        for i in range(len(mipsList)):
            app=copy.deepcopy(mipsList[i])
            app.pop(0)
            yMax=max(yMax,max([float(x) for x in app]))
            plt.plot(rangeToPlot,app,color=lineColors[j],ls=lineStyles[i],marker=lineMarkers[i],label=mipsList[i][0],lw=3.0,ms=10.0)

    # Legends and title
    plt.xticks(rangeToPlot,fontsize=20)
    plt.yticks(fontsize=20)

    plt.ylim(0,yMax+1000)
    plt.xlim(0.8,4.2)
    plt.grid(b=True, which='both', color='0.65',linestyle='-')
    plt.ylabel("Million Instructions per Second (MIPS)",fontsize=20)
    plt.xlabel("# Host Cores",fontsize=20)
    #~ plt.subplots_adjust(left=0.1, right=0.5, top=0.9, bottom=0.1)
    
    #legend ticks
    legend= plt.legend(loc='center', shadow=True, ncol=len(files),fontsize=16,title="(i)       (ii)     (iii)    (iv)     (v)    (vi)                   ",columnspacing=1,bbox_to_anchor=(.5, -.25))
    legend.get_title().set_position((0, 0))
    plt.setp(legend.get_title(), multialignment='left')
    plt.setp(legend.get_title(), fontsize=20)
    
    #~ legend.get_title().set_horizontalalignment('center')


    cnt=0
    for text in legend.texts:
        cnt+=1
        if ((cnt-((len(files)-1)*6) -1) <0):
            text.set_text('')  # disable label

    figureName="speeds"

if options.scalability:
    f = plt.figure(figsize=(14,8))

    # read input
    mipsList=[]
    for line in open(options.replotfile, 'r'):
        mipsList.append(line.split())

    numberOfCores=len(mipsList[0])-1
    rangeToPlot=range(1,numberOfCores+1)

    #plot data
    for i in range(len(mipsList)):
        app=copy.deepcopy(mipsList[i])
        app.pop(0)
        plt.plot(rangeToPlot,[ float(a)/float(app[0]) for a in app],color=lineColors[i],ls=lineStyles[i],marker=lineMarkers[i],label=mipsList[i][0],lw=3.0,ms=10.0)

    # plot XY graph
    plt.plot(rangeToPlot,rangeToPlot,color='black',lw=2,label="XY")

    #~ plt.xlim(0,i)
    #~ plt.ylim(0,y_max*1.25)

    plt.yticks(rangeToPlot,fontsize=20)
    plt.xticks(rangeToPlot,fontsize=20)
    plt.grid(b=True, which='both', color='0.65',linestyle='-')
    plt.ylabel("Speed-up factor (times)",fontsize=20)
    plt.xlabel("# Host Cores",fontsize=20)
    plt.legend(loc='upper left', shadow=True, ncol=1,fontsize=20)

    figureName="scalabilty"

#Output file
if options.outputfile:
    figureName=options.outputfile

plt.savefig("{0}/{1}.eps".format(options.outputfolder,figureName),bbox_inches='tight')
