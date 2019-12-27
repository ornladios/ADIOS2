FROM centos:centos7

# Install core dev packages
RUN yum -y install make curl file git gcc gcc-g++ valgrind vim \
        gdb zlib zlib-devel bzip2 bzip2-libs bzip2-devel python-devel \
        libfabric libfabric-devel
RUN yum -y install epel-release && \
    yum -y install blosc-devel zeromq-devel libzstd libzstd-devel

# Install and setup  newer version of git from the
# Red Hat Software Collections Library
RUN yum -y install centos-release-scl-rh && \
    yum -y install rh-git218
ENV LD_LIBRARY_PATH=/opt/rh/httpd24/root/usr/lib64:${LD_LIBRARY_PATH} \
    PATH=/opt/rh/rh-git218/root/usr/bin:${PATH}

# Install Intel C++ compiler
WORKDIR /tmp
COPY silent-custom.cfg /tmp
RUN curl -L 'https://data.kitware.com/api/v1/file/5c1a96318d777f072bdbff4b/download' | tar -xz \
  && ./parallel_studio_xe_2018_update4_cluster_edition/install.sh -s ./silent-custom.cfg \
  && rm -rf parallel_studio_xe_2018* silent*

# Install extra repos
RUN yum -y install epel-release https://github.com/openhpc/ohpc/releases/download/v1.3.GA/ohpc-release-1.3-1.el7.x86_64.rpm

# Install intel OpenHPC packages
RUN yum -y install \
  lmod-ohpc intel-compilers-devel-ohpc \
  python-numpy-intel-ohpc hdf5-intel-ohpc

# Install the CMake binary
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

# Install ZFP
WORKDIR /opt/zfp
RUN curl -L https://github.com/LLNL/zfp/releases/download/0.5.5/zfp-0.5.5.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
    module load intel && \
    export CC=icc CXX=icpc FC=ifort && \
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
    module load intel && \
    export CC=icc CXX=icpc FC=ifort && \
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/sz/2.1.8.0 ../SZ-2.1.8.0 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf SZ-2.1.8.0 build
ENV PATH=/opt/sz/2.1.8.0/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/sz/2.1.8.0/lib64:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/sz/2.1.8.0:${CMAKE_PREFIX_PATH}

# Misc cleanup of unneeded files
RUN rm -rfv \
        /tmp/* \
        /opt/intel/man \
        /opt/intel/documentation_2018 \
        /opt/intel/ide_support_2018 \
        /opt/intel/samples_2018 \
        /opt/intel/compilers_and_libraries_2018.2.199/linux/mkl/benchmarks \
        /opt/intel/compilers_and_libraries_2018.2.199/linux/mkl/examples \
        /var/cache/yum
