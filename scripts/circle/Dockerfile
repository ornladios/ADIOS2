FROM centos:centos7

# Install extra repos
RUN yum -y install epel-release https://github.com/openhpc/ohpc/releases/download/v1.3.GA/ohpc-release-1.3-1.el7.x86_64.rpm && \
    yum -y upgrade

# Install core dev packages
RUN yum -y install \
      curl file \
      gcc gcc-c++ gcc-gfortran \
      git make autoconf automake libtool\
      bzip2-devel zeromq-devel hdf5-devel \
      python-devel numpy

# Install gcc7 OpenHPC packages
RUN yum -y install \
      lmod-ohpc gnu7-compilers-ohpc openmpi-gnu7-ohpc phdf5-gnu7-openmpi-ohpc \
      python-numpy-gnu7-ohpc

# Install the CMake binary
WORKDIR /opt/cmake/3.6.0
RUN curl -L https://cmake.org/files/v3.6/cmake-3.6.0-Linux-x86_64.tar.gz | \
      tar --strip-components=1 -xzv

# ZFP
WORKDIR /opt/zfp
RUN mkdir build install && \
    git clone https://github.com/LLNL/zfp.git src && \
    cd build && \
    /opt/cmake/3.6.0/bin/cmake \
      -DCMAKE_INSTALL_PREFIX=/opt/zfp/install \
      ../src && \
    make -j8 install && \
    cd ../ && \
    rm -rf build src

# ADIOS1

# Source setup
WORKDIR /opt/adios1/1.12.0
RUN mkdir source && \
    curl -L https://github.com/ornladios/ADIOS/archive/v1.12.0.tar.gz | \
      tar -C source --strip-components=1 -xzv && \
    cd source && ./autogen.sh

# GCC 4.8
RUN cd source && \
    CFLAGS="-O2 -fPIC" CXXFLAGS="-O2 -fPIC" FCFLAGS="-O2 -fPIC" \
      ./configure --prefix=/opt/adios1/1.12.0/gnu48 --without-mpi && \
    make -j8 install && \
    make distclean
    
# GCC 7
RUN cd source && \
    . /etc/profile && \
    module load gnu7 openmpi phdf5 && \
    CFLAGS="-O2 -fPIC" CXXFLAGS="-O2 -fPIC" FCFLAGS="-O2 -fPIC" \
      ./configure --prefix=/opt/adios1/1.12.0/gnu7_openmpi && \
    make -j8 install

# Cleanup
RUN rm -rf /opt/adios1/1.12.0/source

WORKDIR /home/adios2
