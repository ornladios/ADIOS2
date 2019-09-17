FROM fedora:latest

# Install core dev packages
RUN dnf -y install gcc gcc-c++ gcc-gfortran git make curl file patch \
        zlib-devel bzip2 bzip2-libs bzip2-devel openmpi-devel vim valgrind \
        python3-devel python3-numpy python3-mpi4py-openmpi zeromq-devel \
        libubsan

# Cleanup headers and packages
RUN dnf clean all

# Install HDF5 1.10.4 (the current 1.10.5 has a parallel close bug affecting
# the tests
WORKDIR /opt/hdf5
RUN curl -O https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.4/src/hdf5-1.10.4.tar.bz2 && \
  tar -xvf hdf5-1.10.4.tar.bz2 && \
  cd hdf5-1.10.4 && \
  source /etc/profile && \
  module load mpi && \
  ./configure --prefix=/opt/hdf5/1.10.4 --enable-shared --disable-static --enable-parallel && \
  make -j$(grep -c '^processor' /proc/cpuinfo) install && \
  cd .. && \
  rm -rf hdf5-1.10.4.tar.bz2 hdf5-1.10.4

# Create a non-root user to run the builds/tests
RUN export uid=1001 gid=1001 && \
    mkdir -p /home/adios2 && \
    echo "adios2:x:${uid}:${gid}:adios2,,,:/home/adios2:/bin/bash" >> /etc/passwd && \
    echo "adios2:x:${uid}:" >> /etc/group && \
    chown ${uid}:${gid} -R /home/adios2

# Install the CMake binary
WORKDIR /opt/cmake
RUN curl -L https://github.com/Kitware/CMake/releases/download/v3.15.3/cmake-3.15.3-Linux-x86_64.tar.gz | \
    tar -C /opt/cmake --strip-components=1 -xzv

# Install and initialize git-lfs
RUN curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.rpm.sh | bash && \
    dnf install -y git-lfs && \
    runuser -l adios2 -c 'git lfs install'

# Misc cleanup of unneeded files
RUN rm -rfv /tmp/* /var/cache/dnf

USER adios2
ENV HOME /home/adios2
WORKDIR /home/adios2
CMD /bin/bash
