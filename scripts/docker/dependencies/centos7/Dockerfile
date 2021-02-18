FROM centos:centos7

# Install system dependencies
RUN yum upgrade -y && \
    yum install -y epel-release && \
    yum install -y https://repo.ius.io/ius-release-el7.rpm && \
    yum install -y \
        sudo curl vim git224 gdb gcc gcc-c++ gcc-gfortran make pkg-config \
        openmpi-devel blosc-devel bzip2-devel zeromq-devel \
        libzstd-devel hdf5-openmpi-devel cmake3 \
        python-devel numpy mpi4py-openmpi libfabric-devel && \
    ln -s /usr/bin/cmake3 /usr/local/bin/cmake && \
    yum clean all

# Install ZFP
ARG zfp_ver=0.5.5
WORKDIR /opt/zfp
RUN git clone --depth 1 --branch ${zfp_ver} \
        https://github.com/LLNL/zfp.git source && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
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
    echo export LD_LIBRARY_PATH="/opt/zfp/${zfp_ver}/lib64:${LD_LIBRARY_PATH}" \
        >> /etc/profile.d/adios2.sh && \
    echo export CMAKE_PREFIX_PATH="/opt/zfp/${zfp_ver}:${CMAKE_PREFIX_PATH}" \
        >> /etc/profile.d/adios2.sh

# Install SZ
ARG sz_ver=2.1.11.1
WORKDIR /opt/sz
RUN git clone --depth 1 --branch v${sz_ver} \
        https://github.com/disheng222/SZ.git source && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
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
    echo export LD_LIBRARY_PATH="/opt/sz/${sz_ver}/lib64:${LD_LIBRARY_PATH}" \
        >> /etc/profile.d/adios2.sh && \
    echo export CMAKE_PREFIX_PATH="/opt/sz/${sz_ver}:${CMAKE_PREFIX_PATH}" \
        >> /etc/profile.d/adios2.sh

RUN useradd -G wheel -m adios && echo "adios:adios" | chpasswd && \
    echo module load mpi >> /home/adios/.bashrc
WORKDIR /home/adios
USER adios
CMD /bin/bash --login
