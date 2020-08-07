FROM centos:centos7

# Install core dev packages
RUN yum upgrade -y && \
    yum -y install make curl file valgrind vim bison flex sudo gdb \
        pkgconfig bison flex pkgconfig gcc gcc-c++ gcc-gfortran \
        zlib zlib-devel bzip2 bzip2-libs bzip2-devel libpng-devel \
        libfabric-devel libffi-devel python3 python3-devel
RUN yum -y install epel-release && \
    yum -y install zeromq-devel blosc-devel libzstd-devel

# Install and setup newer version of git
RUN yum install -y https://repo.ius.io/ius-release-el7.rpm && \
    yum -y install git224 && \
    yum remove -y ius-release

# Install the most recent CMake nightly binary
WORKDIR /opt/cmake
RUN curl -L https://cmake.org/files/dev/$(curl https://cmake.org/files/dev/ | sed -n '/Linux-x86_64.tar.gz/s/.*>\(cmake[^<]*\)<.*/\1/p' | sort | tail -1) | tar --strip-components=1 -xzv
ENV PATH=/opt/cmake/bin:${PATH}

# Misc cleanup of unneeded files
RUN yum clean all && \
    rm -rfv /tmp/* /var/cache/yum
