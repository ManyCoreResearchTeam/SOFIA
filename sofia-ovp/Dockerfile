FROM ubuntu:18.04

# Prevent dialogs during build
ENV DEBIAN_FRONTEND noninteractive

#Dependencies
RUN apt update && apt-get install -y \
g++-multilib \
gcc-multilib

RUN apt update && apt-get install -y \
cpio \
curl \
g++ \
gcc \
git \
iputils-ping \
lib32ncurses5 \
libncurses5 \
lsb \
python-dev \
python-enum34 \
python-numpy \
python-pip \
python-scipy \
python-setuptools \
python-tk \
python3-dev \
python3-numpy \
python3-pip \
python3-scipy \
python3-setuptools \
python3-tk \
rsync \
sudo \
time \
nano \
vim

RUN apt-get install -y \
g++-aarch64-linux-gnu \
g++-arm-linux-gnueabi \
gcc-aarch64-linux-gnu \
gcc-arm-linux-gnueabi \
gcc-arm-none-eabi \
gfortran-aarch64-linux-gnu \
gfortran-arm-linux-gnueabi

# This must be in a different command otherwise it breaks the build
RUN apt-get install -y gcc-arm-linux-gnueabihf

RUN pip install --upgrade pip numpy matplotlib==1.5.3 pandas enum34

RUN pip3 install --upgrade pip numpy matplotlib==1.5.3 pandas enum34

RUN mkdir -p /usr/tmp

# RUN useradd -m -g users -G sudo -p trocar $my_user

WORKDIR /root
