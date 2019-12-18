FROM centos:centos7

# Install core dev packages
RUN yum -y install make curl file git valgrind vim bison flex sudo gdb \
        pkgconfig bison flex pkgconfig bzip2 bzip2-libs gcc gcc-c++ \
        zlib-devel bzip2-devel libpng-devel libfabric-devel libffi-devel
RUN yum -y install epel-release && \
    yum -y install zeromq-devel blosc-devel python34-devel libzstd libzstd-devel

# Install and setup  newer version of git from the
# Red Hat Software Collections Library
RUN yum -y install centos-release-scl-rh && \
    yum -y install rh-git218
ENV LD_LIBRARY_PATH=/opt/rh/httpd24/root/usr/lib64:${LD_LIBRARY_PATH} \
    PATH=/opt/rh/rh-git218/root/usr/bin:${PATH}

# Install OpenHPC packages
RUN yum -y install https://github.com/openhpc/ohpc/releases/download/v1.3.GA/ohpc-release-1.3-1.el7.x86_64.rpm
RUN yum -y install lmod-ohpc \
        gnu8-compilers-ohpc openmpi3-gnu8-ohpc \
        python34-numpy-gnu8-ohpc python34-mpi4py-gnu8-openmpi3-ohpc

# Install the CMake
WORKDIR /opt/cmake
RUN yum install -y openssl-devel && \
    git clone https://gitlab.kitware.com/cmake/cmake.git source && \
    mkdir build && \
    cd build && \
    ../source/bootstrap --prefix=/opt/cmake --parallel=$(grep -c '^processor' /proc/cpuinfo) && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf source build
ENV PATH=/opt/cmake/bin:${PATH}

# Install HDF5 1.10.4 (the current 1.10.5 has a parallel close bug affecting
# the tests
WORKDIR /opt/hdf5
RUN curl -L https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.4/src/hdf5-1.10.4.tar.bz2 | \
        tar -xvj && \
    cd hdf5-1.10.4 && \
    source /etc/profile && \
    module load gnu8 openmpi3 && \
    export CC=mpicc CXX=mpicxx FC=mpif90 && \
    ./configure --prefix=/opt/hdf5/1.10.4 --enable-shared --disable-static --enable-parallel && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf hdf5-1.10.4
ENV PATH=/opt/hdf5/1.10.4/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/hdf5/1.10.4/lib:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/hdf5/1.10.4:${CMAKE_PREFIX_PATH}

# Install ZFP
WORKDIR /opt/zfp
RUN curl -L https://github.com/LLNL/zfp/releases/download/0.5.5/zfp-0.5.5.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
    module load gnu8 && \
    export CC=gcc CXX=g++ FC=gfortran && \
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/zfp/0.5.5 ../zfp-0.5.5 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf zfp-0.5.5 build
ENV PATH=/opt/zfp/0.5.5/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/zfp/0.5.5/lib64:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/zfp/0.5.5:${CMAKE_PREFIX_PATH}

# Install SZ
WORKDIR /opt/sz
RUN curl -L https://github.com/disheng222/SZ/archive/v2.1.8.0.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
    module load gnu8 && \
    export CC=gcc CXX=g++ FC=gfortran && \
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/sz/2.1.8.0 ../SZ-2.1.8.0 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf SZ-2.1.8.0 build
ENV PATH=/opt/sz/2.1.8.0/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/sz/2.1.8.0/lib64:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/sz/2.1.8.0:${CMAKE_PREFIX_PATH}

# Install git-lfs
RUN curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.rpm.sh | bash && \
    yum install -y git-lfs

# Misc cleanup of unneeded files
RUN yum clean all && \
    rm -rfv /tmp/* /var/cache/yum
