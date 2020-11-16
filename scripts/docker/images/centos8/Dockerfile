###############################################################################
# Dockerfile for ADIOS2 on CentOS
###############################################################################
#
# This is an example Dockerfile image to build and install adios2 from
# source in CentOS including dependencies from apt-get
#
# This accepts a build argument with the following default:
#   adios_ver=master
#
# Modify at your convenience.
#
# Usage examples:
#   ADIOS v2.6.0 on CentOS 8
# 	$ cd /path/to/adios2-src/scripts/docker/images
# 	$ docker build -t adios2:2.6.0-centos8 --build-arg adios_ver=v2.6.0 centos8
###############################################################################

FROM centos:centos8

# Install system dependencies
RUN sed 's|enabled=0|enabled=1|' -i /etc/yum.repos.d/CentOS-PowerTools.repo && \
    yum upgrade -y && \
    yum install -y epel-release && \
    yum install -y \
        sudo curl vim git gdb gcc gcc-c++ gcc-gfortran make pkg-config \
        openmpi-devel blosc-devel bzip2-devel zeromq-devel \
        libzstd-devel fftw-devel hdf5-openmpi-devel \
        python3-devel libfabric-devel python3-mpi4py-openmpi \
        python3-numpy

# Build the latest CMake
WORKDIR /opt/cmake/3.17.0
RUN yum install -y \
        bzip2-devel libcurl-devel expat-devel \
        xz-devel rhash-devel zlib-devel libzstd-devel && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.17.0/cmake-3.17.0.tar.gz | \
        tar -xz && \
    mkdir build && \
    cd build && \
    ../cmake-3.17.0/bootstrap \ 
      --system-libs \
      --no-qt-gui \
      --no-system-libarchive \
      --no-system-libuv \
      --no-system-jsoncpp \
      --prefix=/opt/cmake/3.17.0 \
      --parallel=$(grep -c '^processor' /proc/cpuinfo) && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf cmake-3.17.0 build && \
    echo 'export PATH="/opt/cmake/3.17.0/bin:${PATH}"' \
        >> /etc/profile.d/adios2.sh

# Install ZFP
WORKDIR /opt/zfp
RUN git clone --depth 1 --branch 0.5.5 \
        https://github.com/LLNL/zfp.git source && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
    cmake \
      -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/opt/zfp/0.5.5 \
      -DBUILD_TESTING=OFF \
      ../source && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf source build && \
    echo 'export PATH="/opt/zfp/0.5.5/bin:${PATH}"' \
        >> /etc/profile.d/adios2.sh && \
    echo 'export LD_LIBRARY_PATH="/opt/zfp/0.5.5/lib64:${LD_LIBRARY_PATH}"' \
        >> /etc/profile.d/adios2.sh && \
    echo 'export CMAKE_PREFIX_PATH="/opt/zfp/0.5.5:${CMAKE_PREFIX_PATH}"' \
        >> /etc/profile.d/adios2.sh

# Install SZ
WORKDIR /opt/sz
RUN git clone --depth 1 --branch v2.1.8.3 \
        https://github.com/disheng222/SZ.git source && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
    cmake \
      -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/opt/sz/2.1.8.3 \
      -DBUILD_TESTS=OFF \
      ../source && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf source build && \
    echo 'export PATH="/opt/sz/2.1.8.3/bin:${PATH}"' \
        >> /etc/profile.d/adios2.sh && \
    echo 'export LD_LIBRARY_PATH="/opt/sz/2.1.8.3/lib64:${LD_LIBRARY_PATH}"' \
        >> /etc/profile.d/adios2.sh && \
    echo 'export CMAKE_PREFIX_PATH="/opt/sz/2.1.8.3:${CMAKE_PREFIX_PATH}"' \
        >> /etc/profile.d/adios2.sh

# Install adios
# Note: There's a little extra bit of trickery here to retrieve active pull
# requests not yet merged.  This is only used for CI purposes and can be
# dropped from your own images
ARG adios_ver=master
WORKDIR /opt/adios
RUN git clone https://github.com/ornladios/adios2.git source && \
    cd source && \
    git fetch origin +refs/pull/*/head:refs/remotes/origin/pr* && \
    git checkout ${adios_ver} && \
    cd .. && \
    mkdir build && \
    cd build && \
    adios_ver_dir=$(echo "${adios_ver}" | sed 's|^v\([1-9]\)|\1|') && \
    source /etc/profile && \
    module load mpi && \
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/opt/adios/${adios_ver_dir} \
      -DADIOS2_BUILD_TESTING=OFF \
      -DADIOS2_BUILD_EXAMPLES=OFF \
      ../source && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf source build && \
    echo "export PATH=\"/opt/adios/${adios_ver_dir}/bin:${PATH}\"" \
        >> /etc/profile.d/adios2.sh && \
    echo "export LD_LIBRARY_PATH=\"/opt/adios/${adios_ver_dir}/lib64:${LD_LIBRARY_PATH}\"" \
        >> /etc/profile.d/adios2.sh && \
    echo "export CMAKE_PREFIX_PATH=\"/opt/adios/${adios_ver_dir}:${CMAKE_PREFIX_PATH}\"" \
        >> /etc/profile.d/adios2.sh && \
    echo "export PYTHONPATH=\"/opt/adios/${adios_ver_dir}/lib64/python3.6/site-packages:${PYTHONPATH}\"" \
        >> /etc/profile.d/adios2.sh

RUN useradd -G wheel -m adios && echo "adios:adios" | chpasswd && \
    echo module load mpi >> /home/adios/.bashrc
WORKDIR /home/adios
USER adios
CMD /bin/bash --login
