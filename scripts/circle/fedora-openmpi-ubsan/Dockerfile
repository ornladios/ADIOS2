FROM fedora:latest

# Install core dev packages
RUN dnf -y install gcc gcc-c++ gcc-gfortran cmake git make curl file patch \
        zlib-devel bzip2-devel openmpi-devel hdf5-openmpi-devel vim valgrind \
        python3-devel python3-numpy python3-mpi4py-openmpi zeromq-devel \
        libubsan

# Patch the installed Lmod to properly deal with a CMake shell
RUN curl https://github.com/TACC/Lmod/commit/516a986322ac462876218ce140214824f47f5887.patch | patch -p 2 /usr/share/lmod/7.5.16/libexec/tcl2lua.tcl

# Cleanup headers and packages
RUN dnf clean all

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
RUN rm -rfv /tmp/* /var/cache/dnf

USER adios2
ENV HOME /home/adios2
WORKDIR /home/adios2
CMD /bin/bash
