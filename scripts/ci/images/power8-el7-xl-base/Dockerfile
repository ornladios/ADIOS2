FROM ornladios/adios2:ci-x86_64-power8-el7-base

# Install XL compilers
COPY IBM_XL_C_CPP_*_LINUX_COMMUNITY.tar.gz /tmp
COPY IBM_XL_FORTRAN_*_LINUX_COMMUNITY.tar.gz /tmp
WORKDIR /tmp
RUN mkdir xlc && \
    cd xlc && \
    tar -xf ../IBM_XL_C_CPP_*_LINUX_COMMUNITY.tar.gz && \
    yes 1 | ./install && \
    cd .. && \
    mkdir xlf && \
    cd xlf && \
    tar -xf ../IBM_XL_FORTRAN_*_LINUX_COMMUNITY.tar.gz && \
    yes 1 | ./install && \
    cd .. && \
    rm -rf IBM* xlc xlf
ENV CC=/usr/bin/xlc \
    CXX=/usr/bin/xlc++ \
    FC=/usr/bin/xlf

# Install ZFP
WORKDIR /opt/zfp
RUN curl -L https://github.com/LLNL/zfp/releases/download/0.5.5/zfp-0.5.5.tar.gz | tar -xvz && \
    mkdir build && \
    cd build && \
    cmake \
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
    cmake \
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

# Misc cleanup of unneeded files
RUN rm -rf /tmp/*

# Install Spectrum MPI
# /bin/env IBM_SPECTRUM_MPI_LICENSE_ACCEPT=yes /opt/ibm/spectrum_mpi/lap_ce/bin/accept_spectrum_mpi_license.sh
