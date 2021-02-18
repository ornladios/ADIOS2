ARG ubuntu_ver=18.04
FROM ubuntu:${ubuntu_ver}

# Update apt repos
RUN export DEBIAN_FRONTEND=noninteractive && \
    apt-get update -y && \
    apt-get install -y apt-utils

# Install system dependencies
RUN export DEBIAN_FRONTEND=noninteractive && \
    apt-get install -y \
        sudo git gcc g++ gfortran make pkg-config \
        openmpi-bin libopenmpi-dev python3-dev python3-numpy python3-mpi4py \
        libblosc-dev libbz2-dev libpng-dev libhdf5-openmpi-dev libzmq3-dev \
        libzstd-dev

# Install CMake from the Kitware APT repo to get the latest release
RUN export DEBIAN_FRONTEND=noninteractive && \
    apt-get install -y \
        apt-transport-https ca-certificates gnupg wget && \
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc \
            2>/dev/null | \
        gpg --dearmor - | \
        sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null && \
    . /etc/lsb-release && \
    echo "deb https://apt.kitware.com/ubuntu/ ${DISTRIB_CODENAME} main" \
            > /etc/apt/sources.list.d/kitware.list && \
    apt-get update -y && \
    apt-get install -y cmake

# Clean up
RUN apt-get clean -y

# Install ZFP
ARG zfp_ver=0.5.5
WORKDIR /opt/zfp
RUN git clone --branch ${zfp_ver} \
        https://github.com/LLNL/zfp.git source && \
    mkdir build && \
    cd build && \
    . /etc/profile && \
    cmake \
      -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/opt/zfp/${zfp_ver} \
      -DBUILD_TESTING=OFF \
      ../source && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf source build && \
    echo export PATH="/opt/zfp/${zfp_ver}/bin:${PATH}" \
        >> /etc/profile.d/adios2.sh && \
    echo export LD_LIBRARY_PATH="/opt/zfp/${zfp_ver}/lib:${LD_LIBRARY_PATH}" \
        >> /etc/profile.d/adios2.sh && \
    echo export CMAKE_PREFIX_PATH="/opt/zfp/${zfp_ver}:${CMAKE_PREFIX_PATH}" \
        >> /etc/profile.d/adios2.sh

# Install SZ
ARG sz_ver=2.1.11.1
WORKDIR /opt/sz
RUN git clone --branch v${sz_ver} \
        https://github.com/disheng222/SZ.git source && \
    mkdir build && \
    cd build && \
    . /etc/profile && \
    cmake \
      -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/opt/sz/${sz_ver} \
      -DBUILD_TESTS=OFF \
      ../source && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf source build && \
    echo export PATH="/opt/sz/${sz_ver}/bin:${PATH}" \
        >> /etc/profile.d/adios2.sh && \
    echo export LD_LIBRARY_PATH="/opt/sz/${sz_ver}/lib:${LD_LIBRARY_PATH}" \
        >> /etc/profile.d/adios2.sh && \
    echo export CMAKE_PREFIX_PATH="/opt/sz/${sz_ver}:${CMAKE_PREFIX_PATH}" \
        >> /etc/profile.d/adios2.sh

RUN useradd -G sudo -m adios && echo "adios:adios" | chpasswd
WORKDIR /home/adios
USER adios
CMD /bin/bash --login
