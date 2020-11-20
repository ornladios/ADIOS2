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

# Install NVidia HPC SDK
WORKDIR /tmp/nvhpcsdk-install
COPY nvhpc_2020_209_Linux_x86_64_cuda_11.0.tar.gz .
RUN tar -xzf nvhpc_2020_209_Linux_x86_64_cuda_11.0.tar.gz && \
    cd nvhpc_2020_209_Linux_x86_64_cuda_11.0 && \
    export \
      NVHPC_SILENT=true \
      NVHPC_INSTALL_DIR=/opt/nvidia/hpc_sdk \
      NVHPC_INSTALL_TYPE=single && \
    ./install && \
    echo 'export MODULEPATH=/opt/nvidia/hpc_sdk/modulefiles:${MODULEPATH}' > /etc/profile.d/nvhpc-modules.sh && \
    echo 'setenv MODULEPATH /opt/nvidia/hpc_sdk/modulefiles:${MODULEPATH}' > /etc/profile.d/pgi-modules.csh

# Remove all the CUDA components since we don't need them for CI images
WORKDIR /opt/nvidia/hpc_sdk
RUN rm -rf \
        Linux_x86_64/2020 \
        Linux_x86_64/20.9/cuda \
        Linux_x86_64/20.9/math \
        Linux_x86_64/20.9/profilers \
        Linux_x86_64/20.9/examples \
        Linux_x86_64/20.9/REDIST \
        modulefiles/nvhpc-byo-compiler && \
    sed -e '/cuda/d' -e '/math/d' -i modulefiles/*/*

# Install the most recent CMake nightly binary
WORKDIR /opt/cmake
RUN curl -L https://cmake.org/files/dev/$(curl https://cmake.org/files/dev/ | sed -n '/Linux-x86_64.tar.gz/s/.*>\(cmake[^<]*\)<.*/\1/p' | sort | tail -1) | tar --strip-components=1 -xzv
ENV PATH=/opt/cmake/bin:${PATH}

# Install ZFP
# Note that ZFP needs to be built with GCC at the moment as the results
# are broken when built with PGI
WORKDIR /opt/zfp
RUN curl -L https://github.com/LLNL/zfp/releases/download/0.5.5/zfp-0.5.5.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
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
RUN curl -L https://github.com/szcompressor/SZ/releases/download/v2.1.11/SZ-2.1.11.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
    module load nvhpc-nompi && \
    /opt/cmake/bin/cmake \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/sz/2.1.11 \
         ../SZ-2.1.11 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf SZ-2.1.11 build
ENV PATH=/opt/sz/2.1.11/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/sz/2.1.11/lib64:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/sz/2.1.11:${CMAKE_PREFIX_PATH}

# Misc cleanup of unneeded files
RUN rm -rf /tmp/* && \
    zypper clean
