FROM centos:centos7

# Install core dev packages
RUN yum -y install make curl file git valgrind vim bison flex sudo gdb \
        pkgconfig bison flex pkgconfig gcc gcc-c++ gcc-gfortran \
        zlib zlib-devel bzip2 bzip2-libs bzip2-devel libpng-devel \
        libfabric-devel libffi-devel
RUN yum -y install epel-release && \
    yum -y install zeromq-devel blosc-devel libzstd-devel

# Install and setup newer version of git from the
# Red Hat Software Collections Library
RUN yum -y install centos-release-scl-rh && \
    yum -y install rh-git218
ENV LD_LIBRARY_PATH=/opt/rh/httpd24/root/usr/lib64:${LD_LIBRARY_PATH} \
    PATH=/opt/rh/rh-git218/root/usr/bin:${PATH}

# Install the most recent CMake nightly binary
WORKDIR /opt/cmake
RUN curl -L https://cmake.org/files/dev/$(curl https://cmake.org/files/dev/ | sed -n '/Linux-x86_64.tar.gz/s/.*>\(cmake[^<]*\)<.*/\1/p' | sort | tail -1) | tar --strip-components=1 -xzv
ENV PATH=/opt/cmake/bin:${PATH}

# Misc cleanup of unneeded files
RUN yum clean all && \
    rm -rfv /tmp/* /var/cache/yum
