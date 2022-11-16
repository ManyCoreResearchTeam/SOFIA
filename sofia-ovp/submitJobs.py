#!/usr/bin/env python2
# -*- coding: utf-8 -*-

from argparse import ArgumentParser

import copy
import os
import pp
import subprocess
import sys
import time
import random

ppservers = ()
serverPath = []
localPath = ""
localHost = ""
space = " "
args = None

###############################################################################
#                                AUX FUNCTIONS                                #
###############################################################################


def checkServersDown():
    global ppservers
    servers = copy.copy(ppservers)
    for i in servers:
        try:
            hostname = i
            filename = "/tmp/ppserver.pid"
            newFilename = hostname + ".pid"
            scp = "scp" + space + hostname + ":" + filename
            cmd = scp + space + newFilename
            subprocess.check_call(cmd, shell=True)
        except BaseException:
            print("Server " + i + " is down, removing it from ppservers")
            ppservers = list(ppservers)
            ppservers.remove(i)
            ppservers = tuple(ppservers)


def getSinglePath():
    path = os.getcwd()
    return path


def getHostname():
    hostname = subprocess.check_output("hostname", shell=True)
    hostname = hostname.rstrip("\n")
    return hostname


def splitPath(path):
    ovpFolder = "/"
    path = path.split("/")
    for i in range(1, len(path) - 2):
        ovpFolder += path[i] + "/"
    return ovpFolder


def getLocalPath():
    owd = os.getcwd()
    os.chdir("../..")
    localPath = os.getcwd()
    os.chdir(owd)
    localPath = localPath + "/"
    return localPath


def readErrors(filename):
    errorFile = open(filename, "r")
    pids = []
    pid = errorFile.readline()
    while pid:
        pid.rstrip('\n')
        pids.append(int(pid))
        pid = errorFile.readline()
    return pids


def init():
    global localPath
    global localHost
    global serverPath

    localPath = getLocalPath()
    localHost = getHostname()
    serverPath.append((localHost, localPath))
    serverPath = dict(serverPath)


def getTimeoutTime():
    f = open("timeOutTime", "r")
    timeOut = int(f.read())
    return timeOut if timeOut > 0 else 10

###############################################################################
#                   DEFINE SERVERS & NETWORK COMMUNICATION                    #
###############################################################################


def defServers(servers):
    global ppservers
    if servers == ():
        ppservers = ()
    elif not isinstance(servers, (tuple,)):
        ppservers = (servers, )
    else:
        ppservers = servers


def getPaths():
    global ppservers
    global serverPath
    if ppservers == ():
        print("Only local server defined")
    else:
        checkServersDown()
        for i in ppservers:
            job_server = pp.Server(0, ppservers=(i, ))
            functionJob1 = getSinglePath
            functionJob2 = getHostname
            funcArgs = ()
            subFunc = ()
            libraries = ("subprocess", )
            job1 = job_server.submit(
                functionJob1, funcArgs, subFunc, libraries)
            job2 = job_server.submit(
                functionJob2, funcArgs, subFunc, libraries)
            path = job1()
            path = splitPath(path)
            hostname = job2()
            serverPath.append((hostname, path))


# TODO: change other calls for subprocess to be like this, first setup the
# command in parts. then call the subprocess. please use meaningful variable
# names
def transferWorkspace():
    for i in serverPath:
        server, folder = i
        opt = space + "-r --delete"
        src = space + "../../workspace/" + args.application
        dst = space + server + ":" + folder + "workspace/"
        cmd = "rsync" + opt + src + dst
        subprocess.call(cmd, shell=True)

def transferWorkloads():
    for i in serverPath:
        server, folder = i
        opt = space + "-r --update --delete"
        src = space + "../../workloads/" 
        dst = space + server + ":" + folder + "workloads/"
        cmd = "rsync" + opt + src + dst
        subprocess.call(cmd, shell=True)

def transferFIM():
    for i in serverPath:
        server, folder = i
        opt = space + "-r --delete"
        src = space + "../../support/fim-api/"
        dst = space + server + ":" + folder + "support/fim-api/"
        cmd = "rsync" + opt + src + dst
        subprocess.call(cmd, shell=True)

def transferIntercept():
    for i in serverPath:
        server, folder = i
        opt = space + "-r --delete"
        src = space + "../../platformOP/intercept/"
        dst = space + server + ":" + folder + "platformOP/intercept/"
        cmd = "rsync" + opt + src + dst
        subprocess.call(cmd, shell=True)

def transferModule():
    for i in serverPath:
        server, folder = i
        opt = space + "-r --delete"
        src = space + "../../platformOP/module/"
        dst = space + server + ":" + folder + "platformOP/module/"
        cmd = "rsync" + opt + src + dst
        subprocess.call(cmd, shell=True)

###############################################################################
#                                  GOLD RUN                                   #
###############################################################################


def runGold(application, goldExecute, path, serverPath):
    space = " "
    hostname = getHostname()
    ovpFolder = serverPath[hostname]
    os.chdir(ovpFolder + "/workspace/" + application)
    logfile = open('PlatformLogs/platform-' + str(0), 'w+')
    path = path[:-1]
    ovpFolder = ovpFolder[:-1]
    echoExecuteCmd = "echo \"" + goldExecute + "\""
    changeFolderPath = "sed "'s@" + path + "@" + ovpFolder + "@g'
    cmd = echoExecuteCmd + space + "|" + space + changeFolderPath
    run = subprocess.check_output(cmd, shell=True)
    run = run.rstrip("\n")

    timeBefore = time.time()
    time.clock()
    cmd = "./ovp.sh" + space + run
    gold = subprocess.call(cmd, shell=True, stdout=logfile)
    timeAfter = time.time()
    f = open("timeOutTime", "w+")
    timeOut = (timeAfter - timeBefore) * 2 + 30
    timeOut = int(timeOut)
    f.write(str(timeOut))
    f.close()


def goldTimeOut():
    jobs = []
    for i in ppservers:
        job_server = pp.Server(0, ppservers=(i, ))
        function = runGold
        functionArgs = (
            args.application,
            args.goldExecute,
            localPath,
            serverPath,
        )
        subFunctions = (getHostname, )
        libraries = ("subprocess", "time", )
        jobs.append(
            job_server.submit(
                function,
                functionArgs,
                subFunctions,
                libraries))
    for job in jobs:
        res = job()


###############################################################################
#                                 HARNESS RUN                                 #
###############################################################################

def runHarness(pid, application, execute, path, serverPath):
    space = " "
    hostname = getHostname()
    ovpFolder = serverPath[hostname]
    os.chdir(ovpFolder + "/workspace/" + application)
    logfile = open('PlatformLogs/platform-' + str(pid), 'w+')
    path = path[:-1]
    ovpFolder = ovpFolder[:-1]
    echoExecuteCmd = "echo \"" + execute + "\""
    changeFolderPath = "sed "'s@" + path + "@" + ovpFolder + "@g'
    cmd = echoExecuteCmd + space + "|" + space + changeFolderPath
    run = subprocess.check_output(cmd, shell=True)
    run = run.rstrip("\n")
    run = run + " " + str(pid)

    timeOutTime = str(getTimeoutTime())

    initOvp = "./ovp.sh"
    timeout = "timeout" + space + timeOutTime
    cmd = initOvp + space + timeout + space + run
    subprocess.call(cmd, shell=True, stdout=logfile)


def run():
    job_server = pp.Server(args.localWorkers, ppservers=ppservers)
    jobs = []
    for pid in range(1, args.nFaults + 1):
        function = runHarness
        functionArgs = (
            pid,
            args.application,
            args.runExecute,
            localPath,
            serverPath,
        )
        subFunctions = (getHostname, getTimeoutTime, )
        libraries = ("subprocess", )
        jobs.append(
            job_server.submit(
                function,
                functionArgs,
                subFunctions,
                libraries))
        if len(ppservers) == 0:
            #time.sleep(random.randint(5, 25))
            # ~ time.sleep(1)
            time.sleep(.2)
    for job in jobs:
        res = job()


###############################################################################
#                                 RUN ERRORS                                  #
###############################################################################

def runErrors(pids):
    job_server = pp.Server(args.localWorkers, ppservers=ppservers)
    jobs = []
    for pid in pids:
        function = runHarness
        functionArgs = (
            pid,
            args.application,
            args.runExecute,
            localPath,
            serverPath,
        )
        subFunctions = (getHostname, getTimeoutTime, )
        libraries = ("subprocess", )
        jobs.append(
            job_server.submit(
                function,
                functionArgs,
                subFunctions,
                libraries))

    for job in jobs:
        res = job()


def checkServerLicense(serverPath, application):
    pids = []
    hostname = getHostname()
    ovpFolder = serverPath[hostname]
    os.chdir(ovpFolder + "workspace/" + application + "/PlatformLogs")

    if os.path.isfile('./errors'):
        subprocess.call("rm errors", shell=True)

    subprocess.call(
        "for i in platform-*; do if grep -q \"LIC_FE\" $i; then echo $i | cut -d'-' -f2 >> errors; fi; done",
        shell=True)
    subprocess.call(
        "for i in platform-*; do if grep -q \"LIC_FE\" $i; then rm $i; fi; done",
        shell=True)
    if os.path.isfile('./errors'):
        pids = readErrors("errors")
    return pids


def checkHangs():
    pids = []
    hostname = getHostname()
    ovpFolder = serverPath[hostname]
    os.chdir(ovpFolder + "workspace/")
    if os.path.isfile('./errors'):
        subprocess.call("rm errors", shell=True)
    subprocess.call(
        "grep \"Hang\" " +
        args.application +
        ".* | tr -s ' ' | cut -d' ' -f2 >> errors",
        shell=True)
    if os.path.isfile('./errors'):
        pids = readErrors("errors")
    return pids


def checkLicenseErrors():
    pids = []
    for i in ppservers:
        job_server = pp.Server(0, ppservers=ppservers)
        function = checkServerLicense
        funcArgs = (serverPath, args.application, )
        subFunctions = (getHostname, readErrors, )
        libraries = ("subprocess",)
        job = job_server.submit(function, funcArgs, subFunctions, libraries)
        res = job()
        pids = pids + list(res)
    res = checkServerLicense(serverPath, args.application)
    pids = pids + list(res)
    return pids

###############################################################################
#                                RECOVER DATA                                 #
###############################################################################


def recoverReports(serverPath, server, localPath):
    ovpFolder = serverPath[server]
    os.chdir(localPath + "workspace/" + args.application + "/Reports")
    reportsFolder = "/Reports"
    workspace = "workspace/" + args.application + reportsFolder
    path = ovpFolder + workspace
    scp = "scp" + space + server + ":" + path
    getAllReports = scp + "/*"
    cmd = getAllReports + space + "."
    subprocess.call(cmd, shell=True)


def recoverPlatformLogs(serverPath, server, localPath):
    ovpFolder = serverPath[server]
    os.chdir(localPath + "workspace/" + args.application + "/PlatformLogs")
    platformsFolder = "/PlatformLogs"
    workspace = "workspace/" + args.application + platformsFolder
    path = ovpFolder + workspace
    scp = "scp" + space + server + ":" + path
    getAllPlatforms = scp + "/*"
    cmd = getAllPlatforms + space + "."
    subprocess.call(cmd, shell=True)


def recoverData():
    global serverPath
    global localHost
    global localPath

    for i in ppservers:
        recoverReports(serverPath, i, localPath)
        recoverPlatformLogs(serverPath, i, localPath)

###############################################################################
#                                    MAIN                                     #
###############################################################################


def main():
    global args
    parser = ArgumentParser()

    parser.add_argument(
        '-g',
        '--gold',
        action='store',
        dest='goldExecute',
        required=True,
        help="gold command")

    parser.add_argument(
        '-r',
        '--run',
        action='store',
        dest='runExecute',
        required=True,
        help="fault injection command")

    parser.add_argument(
        '-w',
        '--workers',
        action='store',
        dest='localWorkers',
        default=0,
        type=int,
        help="number of local workers")

    parser.add_argument(
        '-f',
        '--faultsN',
        action='store',
        dest='nFaults',
        type=int,
        required=True,
        help="number of faults")

    parser.add_argument(
        '-a',
        '--application',
        action='store',
        dest='application',
        required=True,
        help="current application name")

    parser.add_argument(
        '-c',
        '--cluster',
        action='store',
        dest='cluster',
        required=True,
        help="pcs on cluster")

    parser.add_argument(
        '-e',
        '--hangErrors',
        action='store',
        dest='hang',
        default=0,
        type=int,
        help="run erroneous hangs")

    args = parser.parse_args()

    if (args.cluster != "NONE"):
        cluster = args.cluster.split(",")
        for i in range(len(cluster)):
            cluster[i] = cluster[i].replace(" ", "")
        cluster = tuple(cluster)
        print("Clusters = ", cluster)
    else:
        cluster = ()
        print("No Cluster defined")

    defServers(cluster)
    getPaths()
    print("Syncing...") 
    transferWorkspace()
    transferWorkloads()
    transferFIM()
    transferIntercept()
    transferModule()
    init()
    print("Running Gold")
    goldTimeOut()
    if (args.hang == 1):
        print("Checking for hangs")
        pidsList = checkHangs()
        print(pidsList)
        print("Running Hangs")
        runErrors(pidsList)
    else:
        print("Running FI")
        run()
   # print("Checking for license errors")
   # pidsList = checkLicenseErrors()
   # while pidsList:
   #     print("Running errors :(")
   #     runErrors(pidsList)
   #     del pidsList[:]
   #     pidsList = checkLicenseErrors()
    print("Recovering Data!")
    recoverData()


if __name__ == "__main__":
    main()
