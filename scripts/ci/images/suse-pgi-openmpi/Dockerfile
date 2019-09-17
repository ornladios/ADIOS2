FROM opensuse/archive:42.3

# Install core dev packages
RUN zypper ref
RUN zypper in -y gcc gcc-c++ gcc-fortran git make curl tar f2c glibc-locale \
  glibc-devel libbz2-devel pkg-config libzmq-devel zlib-devel gdb vim valgrind \
  blosc-devel

# Workaround so pgi can find g77
WORKDIR /usr/bin
RUN ln -s gfortran g77

# Install libfabric
WORKDIR /opt/libfabric
RUN curl -L https://github.com/ofiwg/libfabric/releases/download/v1.6.0/libfabric-1.6.0.tar.bz2 | tar -xj \
  && cd libfabric-1.6.0 \
  && ./configure --prefix=/opt/libfabric/1.6.0 --libdir=/opt/libfabric/1.6.0/lib64 \
  && make -j8 install \
  && cd .. \
  && rm -rf libfabric-1.6.0

# Install the CMake binary
WORKDIR /opt/cmake
RUN curl -L https://github.com/Kitware/CMake/releases/download/v3.15.2/cmake-3.15.2-Linux-x86_64.tar.gz | \
    tar -C /opt/cmake --strip-components=1 -xzv

# Install PGI compiler
WORKDIR /tmp/pgi-install
RUN curl -L 'https://data.kitware.com/api/v1/file/5d682426d35580e6dccfb6a1/download' | tar -xz
RUN export \
      PGI_SILENT=true \
      PGI_ACCEPT_EULA=accept \
      PGI_INSTALL_DIR=/opt/pgi \
      PGI_INSTALL_NVIDIA=false \
      PGI_INSTALL_JAVA=false \
      PGI_INSTALL_MPI=true \
      PGI_MPI_GPU_SUPPORT=false \
  && ./install \
  && echo -e '#%Module1.0\nset ModulesVersion "3.1.3/2019"' > /opt/pgi/modulefiles/openmpi/.version

RUN zypper in -y environment-modules \
  && echo 'export MODULEPATH=/opt/pgi/modulefiles:${MODULEPATH}' > /etc/profile.d/pgi-modules.sh \
  && echo 'setenv MODULEPATH /opt/pgi/modulefiles:${MODULEPATH}' > /etc/profile.d/pgi-modules.csh

# Install HDF5 1.10.4 (the current 1.10.5 has a parallel close bug affecting
# the tests
#WORKDIR /opt/hdf5
#RUN curl -O https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.4/src/hdf5-1.10.4.tar.bz2 && \
#  tar -xvf hdf5-1.10.4.tar.bz2 && \
#  cd hdf5-1.10.4 && \
#  source /etc/profile && \
#  module load pgi openmpi && \
#  export CC=pgcc CXX=pgc++ FC=pgfortran && \
#  ./configure --prefix=/opt/hdf5/1.10.4 --enable-shared --disable-static --enable-parallel && \
#  make -j$(grep -c '^processor' /proc/cpuinfo) install && \
#  cd .. && \
#  rm -rf hdf5-1.10.4.tar.bz2 hdf5-1.10.4

# Misc cleanup of unneeded files
RUN rm -rf /tmp/* \
  && zypper clean
