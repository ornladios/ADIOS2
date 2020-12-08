FROM debian:sid

RUN apt update && \
    apt full-upgrade -y --no-install-recommends && \
    apt install -y --no-install-recommends \
        curl \
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
        python3.8-dev \
        libpython3.8-dev \
        python3.9-dev \
        libpython3.9-dev \
        python3-distutils \
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
        libffi-dev && \
    apt autoremove -y
