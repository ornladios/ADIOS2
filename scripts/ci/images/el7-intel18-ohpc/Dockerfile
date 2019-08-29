FROM centos:centos7

# Install core dev packages
RUN yum -y install make curl file git gcc gcc-g++ valgrind vim \
        gdb zlib zlib-devel bzip2 bzip2-libs bzip2-devel python-devel
RUN yum -y install epel-release
RUN yum -y install blosc-devel zeromq-devel

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

# Cleanup headers and packages
RUN yum clean all

# Install the CMake binary
WORKDIR /opt/cmake
RUN curl -L https://cmake.org/files/v3.15/cmake-3.15.2-Linux-x86_64.tar.gz | \
    tar --strip-components=1 -xz

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
