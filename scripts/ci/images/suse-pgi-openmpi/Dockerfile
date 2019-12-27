FROM opensuse/leap:15.2

# Install core dev packages
RUN zypper addrepo -fc https://download.opensuse.org/repositories/devel:/tools:/scm/openSUSE_Leap_15.2/devel:tools:scm.repo
RUN zypper --gpg-auto-import-keys ref
RUN zypper in -y gcc gcc-c++ gcc-fortran git make curl tar f2c glibc-locale \
  glibc-devel libbz2-devel pkg-config libzmq-devel zlib-devel gdb vim valgrind \
  bzip2 gzip blosc-devel libzstd-devel openssl-devel environment-modules

# Workaround so pgi can find g77
WORKDIR /usr/bin
RUN ln -s gfortran g77

# Install PGI compiler
WORKDIR /tmp/pgi-install
RUN curl -L https://data.kitware.com/api/v1/file/5defe4f7af2e2eed35763524/download | tar -xz
RUN export \
      PGI_SILENT=true \
      PGI_ACCEPT_EULA=accept \
      PGI_INSTALL_DIR=/opt/pgi \
      PGI_INSTALL_NVIDIA=false \
      PGI_INSTALL_JAVA=false \
      PGI_INSTALL_MPI=false \
      PGI_MPI_GPU_SUPPORT=false \
  && ./install

RUN  echo 'export MODULEPATH=/opt/pgi/modulefiles:${MODULEPATH}' > /etc/profile.d/pgi-modules.sh \
  && echo 'setenv MODULEPATH /opt/pgi/modulefiles:${MODULEPATH}' > /etc/profile.d/pgi-modules.csh

# Install CMake
WORKDIR /opt/cmake
RUN git clone https://gitlab.kitware.com/cmake/cmake.git source && \
    mkdir build && \
    cd build && \
    ../source/bootstrap --prefix=/opt/cmake --parallel=$(grep -c '^processor' /proc/cpuinfo) && \
    make -j$(grep -c '^processor' /proc/cpuinfo) all && \
    make install && \
    cd .. && \
    rm -rf source build
ENV PATH=/opt/cmake/bin:${PATH}

# Install OpenMPI
WORKDIR /opt/openmpi
RUN curl -L https://download.open-mpi.org/release/open-mpi/v4.0/openmpi-4.0.2.tar.bz2 | \
        tar -xvj && \
    cd openmpi-4.0.2 && \
    source /etc/profile && \
    module load pgi && \
    export CC=pgcc CXX=pgc++ FC=pgfortran CPP=/usr/bin/cpp && \
    ./configure \
        --prefix=/opt/openmpi/4.0.2 \
        --enable-mpi-cxx \
        --enable-shared \
        --disable-static && \
    make -j$(grep -c '^processor' /proc/cpuinfo) all && \
    make install && \
    cd .. && \
    rm -rf openmpi-4.0.2
ENV PATH=/opt/openmpi/4.0.2/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/openmpi/4.0.2/lib:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/openmpi/4.0.2:${CMAKE_PREFIX_PATH}

# Install HDF5 1.10.4 (the current 1.10.5 has a parallel close bug affecting
# the tests
WORKDIR /opt/hdf5
RUN curl -L https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.4/src/hdf5-1.10.4.tar.bz2 | \
        tar -xvj && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
    module load pgi openmpi && \
    export CC=pgcc CXX=pgc++ FC=pgfortran && \
    /opt/cmake/bin/cmake \
        -DCMAKE_INSTALL_PREFIX=/opt/hdf5/1.10.4 \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DHDF5_ENABLE_PARALLEL=ON \
        -DHDF5_BUILD_CPP_LIB=OFF\
        -DHDF5_BUILD_EXAMPLES=OFF \ 
        -DBUILD_TESTING=OFF \
        -DHDF5_BUILD_TOOLS=OFF \
        ../hdf5-1.10.4 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf hdf5-1.10.4 build
ENV PATH=/opt/hdf5/1.10.4/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/hdf5/1.10.4/lib:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/hdf5/1.10.4:${CMAKE_PREFIX_PATH}

# Install ZFP
WORKDIR /opt/zfp
RUN curl -L https://github.com/LLNL/zfp/releases/download/0.5.5/zfp-0.5.5.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
    module load pgi && \
    export CC=pgcc CXX=pgc++ FC=pgffort && \
    /opt/cmake/bin/cmake \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/zfp/0.5.5 \
        ../zfp-0.5.5 && \
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
    module load pgi && \
    export CC=pgcc CXX=pgc++ FC=pgffort && \
    /opt/cmake/bin/cmake \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/sz/2.1.8.0 \
         ../SZ-2.1.8.0 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf SZ-2.1.8.0 build
ENV PATH=/opt/sz/2.1.8.0/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/sz/2.1.8.0/lib64:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/sz/2.1.8.0:${CMAKE_PREFIX_PATH}

# Misc cleanup of unneeded files
RUN rm -rf /tmp/* \
  && zypper clean
