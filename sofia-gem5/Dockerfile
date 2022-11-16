FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update \
        && apt-get upgrade -y \
        && apt-get install -y \
            build-essential \
            git \
            libboost-all-dev \
            libgoogle-perftools-dev \
            libprotobuf-dev \
            libprotoc-dev \
            m4 \
            pkg-config \
            protobuf-compiler \
            python \
            python-dev \
            python-six \
            scons \
            zlib1g \
            zlib1g-dev \
        && apt-get install -y \
            cpio \
            python-pip \
            rsync \
            sudo \
            telnet \
            time \
            wget \
        && apt-get install -y \
            g++-5 \
            g++-aarch64-linux-gnu \
            g++-arm-linux-gnueabi \
        && apt-get install -y \
            gcc-5 \
            gcc-aarch64-linux-gnu \
            gcc-arm-linux-gnueabi \
            gcc-arm-none-eabi \
            gfortran-aarch64-linux-gnu \
            gfortran-arm-linux-gnueabi \
        && pip install --upgrade pip matplotlib==1.5.3 \
        && apt-get install -y \
            python-tk \
            gettext \
            vim \
            nano

ARG my_user=user
ARG my_uid=9001
ARG my_gid=1024
RUN addgroup --gid $my_gid $my_user || true \
        && useradd -ms /bin/bash --uid $my_uid --gid $my_gid -g sudo $my_user \
        && echo "$my_user:$my_user" | chpasswd \
        && echo "$my_user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
USER $my_user
WORKDIR /home/$my_user/
