#!/bin/bash

: '
""" Fault Injection Module to Gem5
    Version 2.0
    16.12.2016

    Author: Felipe Rocha da Rosa - frdarosa.com
"""
'

INSTALPY=1
INSTALLCROSS=1
INSTALFORTRAN=1

function CMD {
for pckg in $2 ; do
	$1 $pckg
done
}

# Common packages
GCC="g++ g++-multilib lib32ncurses5 build-essential"
TOOLS="scons swig swig3.0 m4 device-tree-compiler zlib1g-dev"

CROSSARMV7="gcc-arm-linux-gnueabi g++-arm-linux-gnueabi"
CROSSARMV8="gcc-aarch64-linux-gnu g++-aarch64-linux-gnu"
CROSSFORTRAN="gfortran-arm-linux-gnueabi gfortran-aarch64-linux-gnu"


# Fault Injection
CMD "sudo apt -y install" "$GCC $TOOLS"

if [ "$INSTALLCROSS" -eq "1" ] ; then
	CMD "sudo apt -y install" "$CROSSARMV7"
	CMD "sudo apt -y install" "$CROSSARMV8"
fi

if [ "$INSTALFORTRAN" -eq "1" ] ; then
	CMD "sudo apt -y install" "$CROSSFORTRAN"
fi

# The python is required by the support tools
if [ "$INSTALPY" -eq "1" ] ; then
	PYTHON="python-dev python-setuptools python-enum34 python-scipy python-numpy python-tk python-pip python3-pip" 
	PIP="matplotlib==1.5.3"
	CMD "sudo apt -y install" "$PYTHON"
	CMD "sudo pip install"    "$PIP"
fi

