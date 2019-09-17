FROM centos:centos7

# Install core dev packages
RUN yum -y install make curl file git valgrind vim bison flex sudo gdb \
        pkgconfig bison flex pkgconfig bzip2 bzip2-libs \
        zlib-devel bzip2-devel libpng-devel libfabric-devel libffi-devel
RUN yum -y install epel-release
RUN yum -y install zeromq-devel blosc-devel python34-devel

# Install OpenHPC packages
RUN yum -y install https://github.com/openhpc/ohpc/releases/download/v1.3.GA/ohpc-release-1.3-1.el7.x86_64.rpm
RUN yum -y install lmod-ohpc \
        gnu8-compilers-ohpc openmpi3-gnu8-ohpc \
        python34-numpy-gnu8-ohpc python34-mpi4py-gnu8-openmpi3-ohpc

# Install HDF5 1.10.4 (the current 1.10.5 has a parallel close bug affecting
# the tests
WORKDIR /opt/hdf5
RUN curl -O https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.4/src/hdf5-1.10.4.tar.bz2 && \
  tar -xvf hdf5-1.10.4.tar.bz2 && \
  cd hdf5-1.10.4 && \
  source /etc/profile && \
  module load gnu8 openmpi3 && \
  export CC=gcc CXX=g++ FC=gfortran && \
  ./configure --prefix=/opt/hdf5/1.10.4 --enable-shared --disable-static --enable-parallel && \
  make -j$(grep -c '^processor' /proc/cpuinfo) install && \
  cd .. && \
  rm -rf hdf5-1.10.4.tar.bz2 hdf5-1.10.4

# Install the CMake binary
WORKDIR /opt/cmake
RUN curl -L https://github.com/Kitware/CMake/releases/download/v3.15.2/cmake-3.15.2-Linux-x86_64.tar.gz | \
    tar -C /opt/cmake --strip-components=1 -xzv

# Install git-lfs
RUN curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.rpm.sh | bash && \
    yum install -y git-lfs

# Misc cleanup of unneeded files
RUN yum clean all && \
    rm -rfv /tmp/* /var/cache/yum
