FROM opensuse/archive:42.3

# Install core dev packages
RUN zypper ref
RUN zypper in -y gcc gcc-c++ gcc-fortran git make curl tar f2c glibc-locale \
  glibc-devel libbz2-devel python-devel python-numpy pkg-config libzmq-devel \
  hdf5-devel zlib-devel vim valgrind gdb blosc-devel libzstd-devel

# Workaround so pgi can find g77
WORKDIR /usr/bin
RUN ln -s gfortran g77

# Install PGI compiler
WORKDIR /tmp/pgi-install
RUN curl -L 'https://data.kitware.com/api/v1/file/5d682426d35580e6dccfb6a1/download' | tar -xz
RUN export \
      PGI_SILENT=true \
      PGI_ACCEPT_EULA=accept \
      PGI_INSTALL_DIR=/opt/pgi \
      PGI_INSTALL_NVIDIA=false \
      PGI_INSTALL_JAVA=false \
      PGI_INSTALL_MPI=false \
      PGI_MPI_GPU_SUPPORT=false \
  && ./install

RUN zypper in -y environment-modules \
  && echo 'export MODULEPATH=/opt/pgi/modulefiles:${MODULEPATH}' > /etc/profile.d/pgi-modules.sh \
  && echo 'setenv MODULEPATH /opt/pgi/modulefiles:${MODULEPATH}' > /etc/profile.d/pgi-modules.csh

# Install the CMake binary
WORKDIR /opt/cmake
RUN curl -L https://github.com/Kitware/CMake/releases/download/v3.15.3/cmake-3.15.3-Linux-x86_64.tar.gz | \
      tar -C /opt/cmake --strip-components=1 -xzv
ENV PATH /opt/cmake/bin:${PATH}

# Install ZFP
WORKDIR /opt/zfp
RUN curl -L https://github.com/LLNL/zfp/releases/download/0.5.5/zfp-0.5.5.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
    module load pgi && \
    export CC=pgcc CXX=pgc++ FC=pgffort && \
    /opt/cmake/bin/cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/zfp/0.5.5 ../zfp-0.5.5 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf zfp-0.5.5 build
ENV PATH /opt/zfp/0.5.5/bin:${PATH}
ENV LD_LIBRARY_PATH /opt/zfp/0.5.5/lib64:${LD_LIBRARY_PATH}
ENV CMAKE_PREFIX_PATH /opt/zfp/0.5.5:${CMAKE_PREFIX_PATH}

# Install SZ
WORKDIR /opt/sz
RUN curl -L https://github.com/disheng222/SZ/archive/v2.1.6.0.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
    module load pgi && \
    export CC=pgcc CXX=pgc++ FC=pgffort && \
    /opt/cmake/bin/cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/sz/2.1.6.0 ../SZ-2.1.6.0 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf SZ-2.1.6.0 build
ENV PATH /opt/sz/2.1.6.0/bin:${PATH}
ENV LD_LIBRARY_PATH /opt/sz/2.1.6.0/lib64:${LD_LIBRARY_PATH}
ENV CMAKE_PREFIX_PATH /opt/sz/2.1.6.0:${CMAKE_PREFIX_PATH}

# Misc cleanup of unneeded files
RUN rm -rf /tmp/* \
  && zypper clean
