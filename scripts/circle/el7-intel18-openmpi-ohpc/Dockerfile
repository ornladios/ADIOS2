FROM centos:centos7

# Install core dev packages
RUN yum -y install epel-release
RUN yum -y install make curl file git gcc gcc-g++ pkg-config valgrind vim \
        gdb zlib zlib-devel bzip2 bzip2-libs bzip2-devel zeromq-devel \
        libfabric-devel

# Install Intel C++ compiler
WORKDIR /tmp/intel-install
COPY silent-custom.cfg /tmp/intel-install
RUN curl -L 'https://data.kitware.com/api/v1/file/5c1a96318d777f072bdbff4b/download' | tar -xz && \
    ./parallel_studio_xe_2018_update4_cluster_edition/install.sh -s ./silent-custom.cfg

# Install extra repos
RUN yum -y install https://github.com/openhpc/ohpc/releases/download/v1.3.GA/ohpc-release-1.3-1.el7.x86_64.rpm

# Install intel OpenHPC packages
RUN yum -y install \
        lmod-ohpc intel-compilers-devel-ohpc python-devel \
        python-numpy-intel-ohpc openmpi3-intel-ohpc \
        python-mpi4py-intel-openmpi3-ohpc phdf5-intel-openmpi3-ohpc \
        netcdf-intel-openmpi3-ohpc adios-intel-openmpi3-ohpc

ENV CC=icc CXX=icpc FC=ifort

# Install the CMake binary
WORKDIR /opt/cmake/3.6.3
RUN curl -L https://cmake.org/files/v3.6/cmake-3.6.3-Linux-x86_64.tar.gz | \
        tar --strip-components=1 -xz

# Install MGARD
RUN git clone -b cmake-cleanup https://github.com/chuckatkins/mgard /opt/mgard/source
WORKDIR /opt/mgard/build
RUN . /etc/profile >/dev/null && \
    module load intel && \
    /opt/cmake/3.6.3/bin/cmake \
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
RUN . /etc/profile >/dev/null && \
    module load intel && \
    /opt/cmake/3.6.3/bin/cmake \
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
    yum install -y git-lfs && \
    runuser -l adios2 -c 'git lfs install'

# Misc cleanup of unneeded files
RUN rm -rfv \
        /tmp/* \
        /opt/mgard/source /opt/mgard/build \
        /opt/zfp/source /opt/zfp/build \
        /opt/intel/man \
        /opt/intel/documentation_2018 \
        /opt/intel/ide_support_2018 \
        /opt/intel/samples_2018 \
        /opt/intel/compilers_and_libraries_2018.2.199/linux/mkl/benchmarks \
        /opt/intel/compilers_and_libraries_2018.2.199/linux/mkl/examples && \
    yum clean all

USER adios2
ENV HOME /home/adios2
WORKDIR /home/adios2
CMD /bin/bash
