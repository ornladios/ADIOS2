FROM centos:centos7.3.1611

RUN yum -y update && \
    yum -y install \
        bzip2-devel.x86_64 \
        curl \
        git \
        gcc \
        gcc-c++ \
        gcc-gfortran \
        gmake \
        hdf5-devel.x86_64 \
        numpy.x86_64 \
        python-devel.x86_64 \
        https://github.com/openhpc/ohpc/releases/download/v1.3.GA/ohpc-release-1.3-1.el7.x86_64.rpm

RUN yum -y update && \
    yum -y install \
        ohpc-base \
        openmpi-gnu7-ohpc \
        phdf5-gnu7-openmpi-ohpc \
        netcdf-gnu7-openmpi-ohpc

WORKDIR /home/adios2

RUN curl -O https://cmake.org/files/v3.6/cmake-3.6.0-Linux-x86_64.tar.gz && \
    mkdir -p /opt/cmake && \
    cd /opt/cmake && \
    tar -zxvf /home/adios2/cmake-3.6.0-Linux-x86_64.tar.gz && \
    rm -rf /home/adios2/cmake-3.6.0-Linux-x86_64.tar.gz && \
    ln -s /opt/cmake/cmake-3.6.0-Linux-x86_64/bin/cmake /usr/local/bin/cmake && \
    ln -s /opt/cmake/cmake-3.6.0-Linux-x86_64/bin/cpack /usr/local/bin/cpack && \
    ln -s /opt/cmake/cmake-3.6.0-Linux-x86_64/bin/ctest /usr/local/bin/ctest && \
    mkdir -p /home/adios2/hidecirclesource

WORKDIR /home/adios2
