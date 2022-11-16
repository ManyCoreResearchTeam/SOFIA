#!/bin/sh

#Load path
export LD_LIBRARY_PATH="usr/lib"
export PATH=$PATH:"/mpich-3.2/bin"

#Debug flag
#~ set -x

APPLICATION=
APP_PATH=

#~ cat /proc/cpuinfo 
cd $APP_PATH

#OpenMP thread AFFINITY
CPUCOUNT=$(grep -c ^processor /proc/cpuinfo)
export GOMP_CPU_AFFINITY=$(seq 0 $((CPUCOUNT-1)))
export OMP_PROC_BIND=TRUE

/root/OVPFIM-ARMv8.elf 0

#Call the application with the arguments
$APPLICATION

#FTP Sent a file
#~ ifconfig eth0 192.168.9.3 up
#~ ping -c 1 192.168.9.4 
#~ ftpput -u frdarosa -p Ecp@194920 192.168.9.4 inverted_image.pgm

echo "The application finished without call the FIM, probably an unexpectedly termination"
#Exit the application in case of seg fault
/root/OVPFIM-ARMv8.elf 2
