#!/bin/bash

: '
""" Fault Injection Module to OVP
    Version 1.0
    02/02/2016

    Author: Felipe Rocha da Rosa - frdarosa.com
"""
'

#Dependencies
sudo apt-get --force-yes -y install git
sudo apt-get --force-yes -y install device-tree-compiler
sudo apt-get --force-yes -y install lib32ncurses5
sudo apt-get --force-yes -y install libncurses5
sudo apt-get --force-yes -y install g++
sudo apt-get --force-yes -y install gcc
sudo apt-get --force-yes -y install gcc-multilib
sudo apt-get --force-yes -y install g++-multilib

#Cross Compiler
# Armv7 C
#sudo apt-get --force-yes -y install gcc-arm-linux-gnueabi
#sudo apt-get --force-yes -y install g++-arm-linux-gnueabi

# Armv7 baremetal
sudo apt-get --force-yes -y install gcc-arm-none-eabi

# Armv8 C
#sudo apt-get --force-yes -y install gcc-aarch64-linux-gnu
#sudo apt-get --force-yes -y install g++-aarch64-linux-gnu

# Armv7 Fortran
#sudo apt-get --force-yes -y install gfortran-arm-linux-gnueabi

# Armv8 Fortran
#sudo apt-get --force-yes -y install gfortran-aarch64-linux-gnu

#Python
sudo apt-get --force-yes -y install python-dev
sudo apt-get --force-yes -y install python-setuptools
#~ sudo apt-get --force-yes -y install python-matplotlib
sudo apt-get --force-yes -y install python-enum34
sudo apt-get --force-yes -y install python-scipy
sudo apt-get --force-yes -y install python-numpy
sudo apt-get --force-yes -y install python-pip
sudo apt-get --force-yes -y install python-tk
pip install matplotlib==1.5.3
pip install --upgrade pip
sudo apt-get install python3-pip
pip2 install --user pp
