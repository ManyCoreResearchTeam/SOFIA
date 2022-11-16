#! /bin/bash
#~ export PATH=/usr/bin:$PATH
#~ export PATH=/home/frdarosa/Downloads/gcc-linaro-aarch64-linux-gnu-4.8-2013.11_linux/bin:$PATH

make clean
autoreconf -i

LINUX_FOLDER="$(pwd)/../Linux"

./configure \
    --host=aarch64-linux-gnu \
    --with-kernel-dir=$LINUX_FOLDER/linux-linaro-tracking-linux-linaro-4.3-2015.11 \
    --with-dtb=$LINUX_FOLDER/multicluster-57x2_53x4.dtb  \
    --with-initrd=$LINUX_FOLDER/initrd.arm64.img
    
make
