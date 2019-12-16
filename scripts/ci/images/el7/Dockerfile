FROM centos:centos7

# Install core dev packages
RUN yum -y install sudo gcc gcc-c++ gcc-gfortran make curl file valgrind \
        vim gdb bison flex pkgconfig \
        zlib-devel bzip2-devel libpng-devel libfabric-devel libffi-devel \
        python-devel numpy
RUN yum -y install epel-release && \
    yum -y install zeromq-devel hdf5 hdf5-devel blosc-devel

# Install and setup  newer version of git from the
# Red Hat Software Collections Library
RUN yum -y install centos-release-scl-rh && \
    yum -y install rh-git218
ENV LD_LIBRARY_PATH=/opt/rh/httpd24/root/usr/lib64:${LD_LIBRARY_PATH} \
    PATH=/opt/rh/rh-git218/root/usr/bin:${PATH}

# Install the CMake binary
WORKDIR /opt/cmake
RUN curl -L https://github.com/Kitware/CMake/releases/download/v3.6.3/cmake-3.6.3-Linux-x86_64.tar.gz | \
    tar -C /opt/cmake --strip-components=1 -xzv
ENV PATH=/opt/cmake/bin:${PATH}

# Install ZFP
WORKDIR /opt/zfp
RUN curl -L https://github.com/LLNL/zfp/releases/download/0.5.5/zfp-0.5.5.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/zfp/0.5.5 ../zfp-0.5.5 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf zfp-0.5.5 build
ENV PATH=/opt/zfp/0.5.5/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/zfp/0.5.5/lib64:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/zfp/0.5.5:${CMAKE_PREFIX_PATH}

# Install SZ
#WORKDIR /opt/sz
#RUN curl -L https://github.com/disheng222/SZ/archive/v2.1.8.0.tar.gz | tar -xvz && \
#    mkdir build && \
#    cd build && \
#    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/sz/2.1.8.0 ../SZ-2.1.8.0 && \
#    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
#    cd .. && \
#    rm -rf SZ-2.1.8.0 build
#ENV PATH /opt/sz/2.1.8.0/bin:${PATH}
#ENV LD_LIBRARY_PATH /opt/sz/2.1.8.0/lib64:${LD_LIBRARY_PATH}
#ENV CMAKE_PREFIX_PATH /opt/sz/2.1.8.0:${CMAKE_PREFIX_PATH}

# Misc cleanup of unneeded files
RUN yum clean all && \
    rm -rfv /tmp/* /var/cache/yum

