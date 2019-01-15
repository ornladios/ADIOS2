FROM fedora:latest

# Install core dev packages
RUN dnf -y install gcc gcc-c++ gcc-gfortran cmake git make curl file patch \
        zlib-devel bzip2-devel openmpi-devel hdf5-openmpi-devel vim valgrind \
        python3-devel python3-numpy python3-mpi4py-openmpi zeromq-devel \
        libubsan libfabric-devel

# Install MGARD
RUN git clone -b cmake-cleanup https://github.com/chuckatkins/mgard /opt/mgard/source
WORKDIR /opt/mgard/build
RUN cmake \
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
RUN cmake \
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
    dnf install -y git-lfs && \
    runuser -l adios2 -c 'git lfs install'

# Misc cleanup of unneeded files
RUN rm -rfv \
        /opt/mgard/source /opt/mgard/build \
        /opt/zfp/source /opt/zfp/build \
        /tmp/* && \
    dnf clean all

USER adios2
ENV HOME /home/adios2
WORKDIR /home/adios2
CMD /bin/bash
