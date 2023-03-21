# vim: ft=dockerfile:
FROM ornladios/adios2:ci-x86_64-power8-el7-base

# Install packages of XL compilers
COPY IBM_XL_C_CPP_*_LINUX_COMMUNITY.tar.gz /tmp
COPY IBM_XL_FORTRAN_*_LINUX_COMMUNITY.tar.gz /tmp
# Upgrades of XL compilers
COPY IBM_XL_C_CPP_*_LINUX.tar.gz /tmp
COPY IBM_XL_FORTRAN_*_LINUX.tar.gz /tmp
WORKDIR /tmp
RUN source /opt/rh/devtoolset-7/enable && \
    mkdir xlc && \
    cd xlc && \
    tar -xf ../IBM_XL_C_CPP_*_LINUX_COMMUNITY.tar.gz && \
    yes 1 | ./install && \
    cd .. && \
    mkdir xlf && \
    cd xlf && \
    tar -xf ../IBM_XL_FORTRAN_*_LINUX_COMMUNITY.tar.gz && \
    yes 1 | ./install && \
    cd .. && \
    mkdir xlc_p && \
    cd xlc_p && \
    tar -xf ../IBM_XL_C_CPP_*_LINUX.tar.gz && \
    yes 1 | ./install && \
    cd .. && \
    mkdir xlf_p && \
    cd xlf_p && \
    tar -xf ../IBM_XL_FORTRAN_*_LINUX.tar.gz && \
    yes 1 | ./install && \
    cd .. && \
    rm -rf /tmp/* && \
    yum -y clean all
ENV CC=/usr/bin/xlc \
    CXX=/usr/bin/xlc++ \
    FC=/usr/bin/xlf

RUN source /opt/rh/devtoolset-7/enable && \
    /opt/ibm/xlC/16.1.1/bin/xlc_configure -gcc /opt/rh/devtoolset-7/root/usr/

ENV XLC_USR_CONFIG=/opt/ibm/xlC/16.1.1/etc/xlc.cfg.centos.7.gcc.7.3.1

# Install ZFP
WORKDIR /opt/zfp
RUN curl -L https://github.com/LLNL/zfp/releases/download/0.5.5/zfp-0.5.5.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
    $LAUNCHER cmake \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_STANDARD=11 \
        -DCMAKE_INSTALL_PREFIX=/opt/zfp/0.5.5 \
        ../zfp-0.5.5 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf zfp-0.5.5 build
ENV PATH=/opt/zfp/0.5.5/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/zfp/0.5.5/lib64:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/zfp/0.5.5:${CMAKE_PREFIX_PATH}

# Install SZ
WORKDIR /opt/sz
RUN curl -L https://github.com/disheng222/SZ/archive/v2.1.8.3.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
    $LAUNCHER cmake \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/sz/2.1.8.3 \
         ../SZ-2.1.8.3 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf SZ-2.1.8.3 build
ENV PATH=/opt/sz/2.1.8.3/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/sz/2.1.8.3/lib64:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/sz/2.1.8.3:${CMAKE_PREFIX_PATH}

# Install HDF5 1.13.0
WORKDIR /opt/hdf5
RUN curl -L https://github.com/HDFGroup/hdf5/archive/refs/tags/hdf5-1_13_0.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
    $LAUNCHER cmake \
        -DCMAKE_INSTALL_PREFIX=/opt/hdf5/1.13.0 \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_STATIC_LIBS=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        -DHDF5_ENABLE_PARALLEL=OFF \
        -DHDF5_BUILD_CPP_LIB=OFF\
        -DHDF5_BUILD_EXAMPLES=OFF \
        -DBUILD_TESTING=OFF \
        -DHDF5_BUILD_TOOLS=OFF \
        ../hdf5-hdf5-1_13_0 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf hdf5-hdf5-1_13_0 build
ENV PATH=/opt/hdf5/1.13.0/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/hdf5/1.13.0/lib:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/hdf5/1.13.0:${CMAKE_PREFIX_PATH}
WORKDIR /root
