FROM centos:centos7

# Install core dev packages
RUN yum -y install gcc gcc-c++ gcc-gfortran git make curl file valgrind vim \
        gdb zlib zlib-devel bzip2 bzip2-libs bzip2-devel python-devel numpy
RUN yum -y install epel-release
RUN yum -y install zeromq-devel hdf5 hdf5-devel

# Cleanup headers and packages
RUN yum clean all

# Install the CMake binary
WORKDIR /opt/cmake/3.6.3
RUN curl -L https://cmake.org/files/v3.6/cmake-3.6.3-Linux-x86_64.tar.gz | \
  tar --strip-components=1 -xz

# Create a non-root user to run the builds/tests
RUN export uid=1001 gid=1001 && \
    mkdir -p /home/adios2 && \
    echo "adios2:x:${uid}:${gid}:adios2,,,:/home/adios2:/bin/bash" >> /etc/passwd && \
    echo "adios2:x:${uid}:" >> /etc/group && \
    chown ${uid}:${gid} -R /home/adios2

# Install and initialize git-lfs
RUN curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.rpm.sh | bash && \
    yum install -y git-lfs && \
    runuser -l adios2 -c 'git lfs install'

# Misc cleanup of unneeded files
RUN rm -rfv /tmp/* /var/cache/yum

USER adios2
ENV HOME /home/adios2
WORKDIR /home/adios2
CMD /bin/bash
