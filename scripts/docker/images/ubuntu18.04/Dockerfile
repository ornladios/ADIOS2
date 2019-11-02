# Dockerfile for ADIOS2 on Ubuntu 18.04 
#####################################################################################################
#
# This is an example Dockerfile image to build and install adios2 master from source in Ubuntu 18.04
# including dependencies from apt-get and (optionally) the gray-scott tutorial example at the end of 
# this file
# Modify at your convenience
# Usage ($ is your terminal prompt): 
# 1) build docker image
# 	$ cd ADIOS2/scripts/docker/images/ubuntu18.04 ; docker build .
#   or:
# 	$ docker build -f /path/to/ADIOS2/scripts/docker/images/ubuntu18.04/Dockerfile .
#   add a tag:
# 	$ docker build -f /path/to/ADIOS2/scripts/docker/images/ubuntu18.04/Dockerfile -t adios2/master
# 2) run the built image in a container
#   $ docker run -i -t ubuntu:18.04 /bin/bash
#   or with tag:
#   $ docker run -i -t adios2/master /bin/bash 
#####################################################################################################  

FROM ubuntu:18.04
MAINTAINER William F Godoy godoywf@ornl.gov
RUN apt-get update -y &&\
    apt-get install build-essential sudo gfortran \
                    openmpi-bin libopenmpi-dev libzmq3-dev \
                    python3-dev python3-mpi4py python3-numpy python3-pip \
                    libblosc-dev libbz2-dev libpng-dev \
                    libhdf5-openmpi-dev \
                    cmake \
                    git vim \
                    -y

RUN cd /tmp &&\
    mkdir -p Software

RUN cd /tmp/Software &&\
    git clone https://github.com/LLNL/zfp.git --single-branch --branch 0.5.5 --depth 1 &&\
    mkdir -p zfp-build &&\
    cd zfp-build &&\
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/zfp -DBUILD_EXAMPLES=OFF ../zfp &&\
    make -j &&\
    make install
    
RUN cd /tmp/Software &&\
    git clone https://github.com/disheng222/SZ.git --single-branch --branch v2.1.5.0 --depth 1 &&\
    cd SZ &&\
    ./configure --prefix=/opt/sz &&\
    make -j &&\
    make install

RUN cd /tmp/Software &&\
    git clone https://github.com/ornladios/ADIOS2.git --single-branch --branch master --depth 1 &&\
    mkdir -p ADIOS2-build &&\
    cd ADIOS2-build &&\
    cmake -DCMAKE_INSTALL_PREFIX=/opt/adios2 \
          -DZFP_ROOT=/opt/zfp \
          -DSZ_ROOT=/opt/sz \
          -DADIOS2_BUILD_TESTING=OFF \
          -DADIOS2_BUILD_EXAMPLES=OFF \
          ../ADIOS2 &&\
    make -j &&\
    make install
    
RUN cd /tmp &&\
    rm -fr Software

ENV PYTHONPATH=/usr/local/lib/python3.6/site-packages/:/opt/adios2/lib/python3.6/site-packages
ENV PATH=/opt/adios2/bin
ENV LD_LIBRARY_PATH=/usr/local/lib:/opt/zfp/lib:/opt/sz/lib:/opt/adios2/lib

# optionally add a new user and start using adios2 with an example
# RUN useradd -m wfg && echo "wfg:wfg" | chpasswd && adduser wfg sudo
# USER wfg
# CMD /bin/bash

# RUN cd ~ &&\
#    git clone https://github.com/pnorbert/adiosvm.git --single-branch --branch master --depth 1 &&\
#    cd adiosvm/Tutorial/gray-scott &&\
#    mkdir -p build &&\
#    cd build &&\
#    cmake -DCMAKE_BUILD_TYPE=Release .. &&\
#    make &&\
#    cd ..
