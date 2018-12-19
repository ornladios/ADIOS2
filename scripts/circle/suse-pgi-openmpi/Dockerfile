FROM opensuse:42.3

# Install core dev packages
RUN zypper ref
RUN zypper in -y gcc gcc-c++ gcc-fortran git make curl tar f2c glibc-locale \
  glibc-devel libbz2-devel pkg-config libzmq-devel zlib-devel gdb vim valgrind

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
WORKDIR /opt/cmake/3.13.1
RUN curl -L https://cmake.org/files/v3.13/cmake-3.13.1-Linux-x86_64.tar.gz | \
     tar --strip-components=1 -xz \
  && pushd /usr/local/bin \
  && ln -s /opt/cmake/3.13.1/bin/ctest \
  && ln -s /opt/cmake/3.13.1/bin/cmake \
  && ln -s /opt/cmake/3.13.1/bin/cpack

# Install PGI compiler
WORKDIR /tmp/pgi-install
RUN curl -L 'https://data.kitware.com/api/v1/file/5c1a8caa8d777f072bdbfafb/download' | tar -xz
RUN export \
      PGI_SILENT=true \
      PGI_ACCEPT_EULA=accept \
      PGI_INSTALL_DIR=/opt/pgi \
      PGI_INSTALL_NVIDIA=false \
      PGI_INSTALL_JAVA=false \
      PGI_INSTALL_MPI=true \
      PGI_MPI_GPU_SUPPORT=false \
  && ./install

RUN zypper in -y environment-modules \
  && echo 'export MODULEPATH=/opt/pgi/modulefiles:${MODULEPATH}' > /etc/profile.d/pgi-modules.sh \
  && echo 'setenv MODULEPATH /opt/pgi/modulefiles:${MODULEPATH}' > /etc/profile.d/pgi-modules.csh

# Create a non-root user to run the builds/tests
RUN export uid=1001 gid=1001 && \
    mkdir -p /home/adios2 && \
    echo "adios2:x:${uid}:${gid}:adios2,,,:/home/adios2:/bin/bash" >> /etc/passwd && \
    echo "adios2:x:${uid}:" >> /etc/group && \
    chown ${uid}:${gid} -R /home/adios2

# Install and initialize git-lfs
RUN curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.rpm.sh | bash && \
    zypper in -y git-lfs && \
    runuser -l adios2 -c 'git lfs install'

# Misc cleanup of unneeded files
RUN rm -rf /tmp/* \
  && zypper clean

USER adios2
ENV HOME /home/adios2
WORKDIR /home/adios2
CMD /bin/bash
