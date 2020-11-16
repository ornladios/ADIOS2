FROM opensuse/leap:15.2

# Install core dev packages
RUN zypper addrepo -fc https://download.opensuse.org/repositories/devel:/tools:/scm/openSUSE_Leap_15.2/devel:tools:scm.repo
RUN zypper --gpg-auto-import-keys ref
RUN zypper in -y gcc gcc-c++ gcc-fortran git make curl tar f2c glibc-locale \
  glibc-devel libbz2-devel pkg-config zeromq-devel zlib-devel gdb vim valgrind \
  bzip2 gzip blosc-devel libzstd-devel libopenssl-devel Modules

# Workaround so pgi can find g77
WORKDIR /usr/bin
RUN ln -s gfortran g77

# Install PGI compiler
WORKDIR /tmp/pgi-install
COPY pgilinux-2019-1910-x86-64.tar.gz /tmp/pgi-install
RUN tar -xzf pgilinux-2019-1910-x86-64.tar.gz && \
    export \
      PGI_SILENT=true \
      PGI_ACCEPT_EULA=accept \
      PGI_INSTALL_DIR=/opt/pgi \
      PGI_INSTALL_NVIDIA=false \
      PGI_INSTALL_JAVA=false \
      PGI_INSTALL_MPI=false \
      PGI_MPI_GPU_SUPPORT=false && \
    ./install && \
    echo 'export MODULEPATH=/opt/pgi/modulefiles:${MODULEPATH}' > /etc/profile.d/pgi-modules.sh && \
    echo 'setenv MODULEPATH /opt/pgi/modulefiles:${MODULEPATH}' > /etc/profile.d/pgi-modules.csh

# Install the most recent CMake nightly binary
WORKDIR /opt/cmake
RUN curl -L https://cmake.org/files/dev/$(curl https://cmake.org/files/dev/ | sed -n '/Linux-x86_64.tar.gz/s/.*>\(cmake[^<]*\)<.*/\1/p' | sort | tail -1) | tar --strip-components=1 -xzv
ENV PATH=/opt/cmake/bin:${PATH}

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
RUN curl -L https://github.com/disheng222/SZ/archive/v2.1.8.3.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
    module load pgi && \
    export CC=pgcc CXX=pgc++ FC=pgffort && \
    /opt/cmake/bin/cmake \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/sz/2.1.8.3 \
         ../SZ-2.1.8.3 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf SZ-2.1.8.3 build
ENV PATH=/opt/sz/2.1.8.3/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/sz/2.1.8.3/lib64:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/sz/2.1.8.3:${CMAKE_PREFIX_PATH}

# Misc cleanup of unneeded files
RUN rm -rf /tmp/* && \
    zypper clean
