FROM opensuse:42.3

# Install core dev packages
RUN zypper ref
RUN zypper in -y gcc gcc-c++ gcc-fortran git make curl tar f2c glibc-locale \
        glibc-devel libbz2-devel pkg-config libzmq-devel zlib-devel vim \
        valgrind gdb autoconf automake libtool

# Workaround so pgi can find g77
WORKDIR /usr/bin
RUN ln -s gfortran g77

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

# Install the CMake binary
WORKDIR /opt/cmake/3.13.1
RUN curl -L https://cmake.org/files/v3.13/cmake-3.13.1-Linux-x86_64.tar.gz | \
     tar --strip-components=1 -xz \
  && ln -t /usr/local/bin -s /opt/cmake/3.13.1/bin/cmake \
  && ln -t /usr/local/bin -s /opt/cmake/3.13.1/bin/ctest \
  && ln -t /usr/local/bin -s /opt/cmake/3.13.1/bin/cpack

# Install libfabric
WORKDIR /opt/libfabric/source
RUN git clone https://github.com/ofiwg/libfabric.git $PWD && \
    ./autogen.sh && \
    ./configure --prefix=/opt/libfabric/latest && \
    make install && \
    echo 'export CMAKE_PREFIX_PATH=/opt/libfabric/latest:${CMAKE_PREFIX_PATH}' >> /etc/profile.d/libfabric.sh && \
    echo 'export LD_LIBRARY_PATH=/opt/libfabric/latest/lib:${LD_LIBRARY_PATH_PATH}' >> /etc/profile.d/libfabric.sh

# Install MGARD
RUN git clone -b cmake-cleanup https://github.com/chuckatkins/mgard /opt/mgard/source
WORKDIR /opt/mgard/build
RUN /opt/cmake/3.13.1/bin/cmake \
        -DBUILD_TESTING=OFF \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_INSTALL_PREFIX=/opt/mgard/latest \
        /opt/mgard/source && \
    make install && \
    echo 'export MGARD_ROOT=/opt/mgard/latest' >> /etc/profile.d/mgard.sh && \
    echo 'export CMAKE_PREFIX_PATH=/opt/mgard/latest:${CMAKE_PREFIX_PATH}' >> /etc/profile.d/mgard.sh && \
    echo 'export LD_LIBRARY_PATH=/opt/mgard/latest/lib:${LD_LIBRARY_PATH_PATH}' >> /etc/profile.d/mgard.sh

# Install ZFP
RUN git clone https://github.com/llnl/zfp /opt/zfp/source
WORKDIR /opt/zfp/build
RUN /opt/cmake/3.13.1/bin/cmake \
        -DBUILD_TESTING=OFF \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_INSTALL_PREFIX=/opt/zfp/latest \
        /opt/zfp/source && \
    make install && \
    echo 'export ZFP_ROOT=/opt/zfp/latest' >> /etc/profile.d/zfp.sh && \
    echo 'export CMAKE_PREFIX_PATH=/opt/zfp/latest:${CMAKE_PREFIX_PATH}' >> /etc/profile.d/zfp.sh && \
    echo 'export LD_LIBRARY_PATH=/opt/zfp/latest/lib64:${LD_LIBRARY_PATH_PATH}' >> /etc/profile.d/zfp.sh

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
       /opt/fabric/source \
       /opt/mgard/source /opt/mgard/build \
       /opt/zfp/source /opt/zfp/build && \
    zypper clean

USER adios2
ENV HOME /home/adios2
WORKDIR /home/adios2
CMD /bin/bash
