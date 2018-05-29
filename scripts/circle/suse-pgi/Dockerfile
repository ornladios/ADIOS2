FROM opensuse:42.3

# Install core dev packages
RUN zypper ref
RUN zypper in -y gcc gcc-c++ gcc-fortran git make curl tar f2c glibc-locale \
  glibc-devel libbz2-devel python-devel python-numpy pkg-config libzmq-devel \
  hdf5-devel

# Workaround so pgi can find g77
WORKDIR /usr/bin
RUN ln -s gfortran g77

# Install the CMake binary
WORKDIR /opt/cmake/3.11.2
RUN curl -L https://cmake.org/files/v3.11/cmake-3.11.2-Linux-x86_64.tar.gz | \
     tar --strip-components=1 -xz \
  && pushd /usr/local/bin \
  && ln -s /opt/cmake/3.11.2/bin/ctest \
  && ln -s /opt/cmake/3.11.2/bin/cmake \
  && ln -s /opt/cmake/3.11.2/bin/cpack

# Install PGI compiler
WORKDIR /tmp/pgi-install
RUN curl -L 'https://data.kitware.com/api/v1/file/5aff14c18d777f15ebe1ec22/download' | tar -xz
RUN export \
      PGI_SILENT=true \
      PGI_ACCEPT_EULA=accept \
      PGI_INSTALL_DIR=/opt/pgi \
      PGI_INSTALL_NVIDIA=false \
      PGI_INSTALL_JAVA=false \
      PGI_INSTALL_MPI=false \
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

# Misc cleanup of unneeded files
RUN rm -rf /tmp/* \
  && zypper clean

USER adios2
ENV HOME /home/adios2
WORKDIR /home/adios2
CMD /bin/bash
