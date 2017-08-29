FROM centos:centos7.3.1611

RUN yum -y update && \
    yum -y install \
        bzip2-devel.x86_64 \
        curl \
        gcc \
        gcc-c++ \
        gcc-gfortran \
        git \
        gmake \
        hdf5-devel.x86_64 \
        numpy.x86_64 \
        python-devel.x86_64 \
        https://github.com/openhpc/ohpc/releases/download/v1.3.GA/ohpc-release-1.3-1.el7.x86_64.rpm

RUN yum -y update && \
    yum -y install \
        czmq-devel \
        mpi4py-openmpi.x86_64 \
        ohpc-base \
        openmpi-gnu7-ohpc \
        phdf5-gnu7-openmpi-ohpc \
        netcdf-gnu7-openmpi-ohpc

WORKDIR /home/adios2

# Now get some packages from source and build them, starting with CMake

RUN curl -O https://cmake.org/files/v3.6/cmake-3.6.0-Linux-x86_64.tar.gz && \
    mkdir -p /opt/cmake && \
    cd /opt/cmake && \
    tar -zxvf /home/adios2/cmake-3.6.0-Linux-x86_64.tar.gz && \
    rm -rf /home/adios2/cmake-3.6.0-Linux-x86_64.tar.gz && \
    ln -s /opt/cmake/cmake-3.6.0-Linux-x86_64/bin/cmake /usr/local/bin/cmake && \
    ln -s /opt/cmake/cmake-3.6.0-Linux-x86_64/bin/cpack /usr/local/bin/cpack && \
    ln -s /opt/cmake/cmake-3.6.0-Linux-x86_64/bin/ctest /usr/local/bin/ctest && \

# Clone and build zfp master

    mkdir /opt/zfp && \
    cd /opt/zfp && \
    mkdir build install && \
    git clone https://github.com/LLNL/zfp.git src && \
    cd build && \
    cmake -DCMAKE_INSTALL_PREFIX:String=/opt/zfp/install ../src/ && \
    make install && \
    cd ../ && \
    rm -rf build src && \

# Clone ADIOS1 and build two versions

    mkdir -p /opt/adios1/1.12.0 && \
    cd /opt/adios1/1.12.0 && \
    curl -OL https://github.com/ornladios/ADIOS/archive/v1.12.0.tar.gz && \
    tar -zxvf v1.12.0.tar.gz && \
    
    mkdir build-gnu-485 build-gnu-710 && \
    mkdir gnu-4.8.5 gnu-7.1.0 && \
    cd build-gnu-485 && \

    cd /opt/adios1/1.12.0 && \
    rm -rf ADIOS-1.12.0 build-gnu-485 build-gnu-710 v1.12.0.tar.gz && \

# Make the directory where CircleCI can clone the ADIOS2 source

    mkdir -p /home/adios2/hidecirclesource

WORKDIR /home/adios2
