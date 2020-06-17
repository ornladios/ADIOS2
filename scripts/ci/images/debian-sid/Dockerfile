FROM debian:sid

RUN apt-get update && \
    apt-get dist-upgrade -y --no-install-recommends && \
    apt-get install -y --no-install-recommends \
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

# Just for the purposes of CI, grab a newer CMake to allow test re-runs
RUN mkdir -p /opt/cmake && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.17.3/cmake-3.17.3-Linux-x86_64.tar.gz | \
    tar -C /opt/cmake --strip-components=1 -xzv
