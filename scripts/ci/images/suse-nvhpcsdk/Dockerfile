FROM ornladios/adios2:ci-suse-nvhpcsdk-base

# Install HDF5
WORKDIR /opt/hdf5
RUN curl -L https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.13/hdf5-1.13.0/src/hdf5-1.13.0.tar.bz2 | \
        tar -xvj && \
    mkdir build && \
    cd build && \
    source /etc/profile && \
    module load nvhpc-nompi && \
    /opt/cmake/bin/cmake \
        -DCMAKE_INSTALL_PREFIX=/opt/hdf5/1.13.0 \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_STATIC_LIBS=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        -DHDF5_ENABLE_PARALLEL=OFF \
        -DHDF5_BUILD_CPP_LIB=OFF\
        -DHDF5_BUILD_EXAMPLES=OFF \ 
        -DBUILD_TESTING=OFF \
        -DHDF5_BUILD_TOOLS=OFF \
        ../hdf5-1.13.0 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd .. && \
    rm -rf hdf5-1.13.0 build
ENV PATH=/opt/hdf5/1.13.0/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/hdf5/1.13.0/lib:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/hdf5/1.13.0:${CMAKE_PREFIX_PATH}

# Misc cleanup of unneeded files
RUN rm -rf /tmp/* && \
    zypper clean
