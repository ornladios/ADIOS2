# Use latest Sid
FROM debian:sid

# Install base dependencies
RUN apt-get update && \
    apt-get dist-upgrade -y --no-install-recommends && \
    apt-get install -y --no-install-recommends \
        devscripts \
        pristine-tar \
        curl \
        python3-all \
        ca-certificates \
        devscripts \
        git \
        cmake \
        ninja-build \
        make \
        g++ \
        gfortran \
        pkg-config \
        libpugixml-dev \
        libyaml-cpp-dev \
        pybind11-dev \
        libgtest-dev \
        nlohmann-json3-dev \
        libpython3-dev \
        python3-numpy \
        python3-mpi4py \
        libblosc-dev \
        libbz2-dev \
        libpng-dev \
        libczmq-dev \
        libopenmpi-dev \
        libhdf5-serial-dev \
        libhdf5-openmpi-dev \
        libfabric-dev \
        libffi-dev

# Build dh-cmake
RUN git clone https://gitlab.kitware.com/debian/dh-cmake.git /root/dh-cmake && \
    apt-get build-dep -y --no-install-recommends /root/dh-cmake && \
    cd /root/dh-cmake && \
    debuild -us -uc && \
    apt-get install -y --no-install-recommends \
        /root/dh-cmake_*.deb && \
    rm -r /root/dh-cmake

# Build libatl
RUN git clone https://gitlab.kitware.com/debian/libatl.git /root/libatl && \
    apt-get build-dep -y --no-install-recommends /root/libatl && \
    cd /root/libatl && \
    pristine-tar checkout ../libatl_$(dpkg-parsechangelog -l debian/changelog -S Version | sed -E 's/^(.*)-[^-]*$/\1/').orig.tar.gz && \
    debuild -us -uc && \
    apt-get install -y --no-install-recommends \
        /root/libatl_*.deb \
        /root/libatl-dev_*.deb && \
    rm -r /root/libatl

# Build libdill
RUN git clone https://gitlab.kitware.com/debian/libdill.git /root/libdill && \
    apt-get build-dep -y --no-install-recommends /root/libdill && \
    cd /root/libdill && \
    pristine-tar checkout ../libdill_$(dpkg-parsechangelog -l debian/changelog -S Version | sed -E 's/^(.*)-[^-]*$/\1/').orig.tar.gz && \
    debuild -us -uc && \
    apt-get install -y --no-install-recommends \
        /root/libdill_*.deb \
        /root/libdill-dev_*.deb && \
    rm -r /root/libdill

# Build libffs
RUN git clone https://gitlab.kitware.com/debian/libffs.git /root/libffs && \
    apt-get build-dep -y --no-install-recommends /root/libffs && \
    cd /root/libffs && \
    pristine-tar checkout ../libffs_$(dpkg-parsechangelog -l debian/changelog -S Version | sed -E 's/^(.*)-[^-]*$/\1/').orig.tar.gz && \
    debuild -us -uc && \
    apt-get install -y --no-install-recommends \
        /root/libffs_*.deb \
        /root/libffs-dev_*.deb && \
    rm -r /root/libffs

# Build gtkorvo-libenet
RUN git clone https://gitlab.kitware.com/debian/gtkorvo-libenet.git /root/gtkorvo-libenet && \
    apt-get build-dep -y --no-install-recommends /root/gtkorvo-libenet && \
    cd /root/gtkorvo-libenet && \
    pristine-tar checkout ../gtkorvo-libenet_$(dpkg-parsechangelog -l debian/changelog -S Version | sed -E 's/^(.*)-[^-]*$/\1/').orig.tar.gz && \
    debuild -us -uc && \
    apt-get install -y --no-install-recommends \
        /root/gtkorvo-libenet1_*.deb \
        /root/gtkorvo-libenet-dev_*.deb && \
    rm -r /root/gtkorvo-libenet

# Build libevpath
RUN git clone https://gitlab.kitware.com/debian/libevpath.git /root/libevpath && \
    apt-get build-dep -y --no-install-recommends /root/libevpath && \
    cd /root/libevpath && \
    pristine-tar checkout ../libevpath_$(dpkg-parsechangelog -l debian/changelog -S Version | sed -E 's/^(.*)-[^-]*$/\1/').orig.tar.gz && \
    debuild -us -uc && \
    apt-get install -y --no-install-recommends \
        /root/libevpath_*.deb \
        /root/libevpath-dev_*.deb && \
    rm -r /root/libevpath
