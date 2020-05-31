###############################################################################
# Dockerfile for ADIOS2 on Ubuntu
###############################################################################
#
# This is an example Dockerfile image to build and install adios2 from
# source in Ubuntu including dependencies from apt-get
#
# This accepts two build arguments with the following defaults:
#   ubuntu_ver=18.04
#   adios_ver=master
#
# Modify at your convenience.
#
# Usage examples:
#   ADIOS master on Ubuntu 18.04
# 	$ cd /path/to/adios2-src/scripts/docker/images
# 	$ docker build -f ubuntu/Dockerfile -t adios2:master-ubuntu1804
#
#   ADIOS v2.6.0 on Ubuntu 19.10
# 	$ cd /path/to/adios2-src/scripts/docker/images
# 	$ docker build -f ubuntu/Dockerfile -t adios2:2.6.0-ubuntu1910 --build-arg ubuntu_ver=19.10 --build-arg adios_ver=v2.6.0
###############################################################################

ARG ubuntu_ver=18.04
FROM ubuntu:${ubuntu_ver}

# Install system dependencies
RUN apt update -y && \
    apt upgrade -y && \
    apt install -y software-properties-common \
        sudo curl vim git gdb build-essential gfortran pkg-config \
        openmpi-bin libopenmpi-dev python3-dev python3-numpy python3-mpi4py \
        libblosc-dev libbz2-dev libpng-dev libhdf5-openmpi-dev libzmq3-dev \
        libzstd-dev libfftw3-dev

# Build the latest CMake
WORKDIR /opt/cmake/3.17.0
RUN apt install -y \
        libbz2-dev libcurl4-openssl-dev libexpat-dev libjsoncpp-dev \
        liblzma-dev librhash-dev libz-dev libzstd-dev && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.17.0/cmake-3.17.0.tar.gz | \
        tar -xz && \
    mkdir build && \
    cd build && \
    ../cmake-3.17.0/bootstrap \ 
      --system-libs \
      --no-qt-gui \
      --no-system-libarchive \
      --no-system-libuv \
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
    . /etc/profile && \
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
    echo 'export LD_LIBRARY_PATH="/opt/zfp/0.5.5/lib:${LD_LIBRARY_PATH}"' \
        >> /etc/profile.d/adios2.sh && \
    echo 'export CMAKE_PREFIX_PATH="/opt/zfp/0.5.5:${CMAKE_PREFIX_PATH}"' \
        >> /etc/profile.d/adios2.sh

# Install SZ
WORKDIR /opt/sz
RUN git clone --depth 1 --branch v2.1.8.3 \
        https://github.com/disheng222/SZ.git source && \
    mkdir build && \
    cd build && \
    . /etc/profile && \
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
    echo 'export LD_LIBRARY_PATH="/opt/sz/2.1.8.3/lib:${LD_LIBRARY_PATH}"' \
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
    . /etc/profile && \
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
    echo "export LD_LIBRARY_PATH=\"/opt/adios/${adios_ver_dir}/lib:${LD_LIBRARY_PATH}\"" \
        >> /etc/profile.d/adios2.sh && \
    echo "export CMAKE_PREFIX_PATH=\"/opt/adios/${adios_ver_dir}:${CMAKE_PREFIX_PATH}\"" \
        >> /etc/profile.d/adios2.sh && \
    echo "export PYTHONPATH=\"/opt/adios/${adios_ver_dir}/lib/python3/dist-packages:${PYTHONPATH}\"" \
        >> /etc/profile.d/adios2.sh

RUN useradd -G sudo -m adios && echo "adios:adios" | chpasswd
WORKDIR /home/adios
USER adios
CMD /bin/bash --login
