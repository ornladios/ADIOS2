FROM centos:centos7

# Install core dev packages
RUN yum -y install sudo gcc gcc-c++ gcc-gfortran git make curl file valgrind \
        vim gdb bison flex pkgconfig \
        zlib-devel bzip2-devel libpng-devel libfabric-devel libffi-devel \
        python-devel numpy
RUN yum -y install epel-release
RUN yum -y install zeromq-devel hdf5 hdf5-devel blosc-devel

# Install the CMake binary
WORKDIR /opt/cmake
RUN curl -L https://github.com/Kitware/CMake/releases/download/v3.6.3/cmake-3.6.3-Linux-x86_64.tar.gz | \
    tar -C /opt/cmake --strip-components=1 -xzv

# Install git-lfs
RUN curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.rpm.sh | bash && \
    yum install -y git-lfs

# Misc cleanup of unneeded files
RUN yum clean all && \
    rm -rfv /tmp/* /var/cache/yum
