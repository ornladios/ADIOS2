FROM ubuntu:22.04

ARG SPACK_VERSION=v1.0.2
ARG SPACK_MIRROR="https://binaries.spack.io/v2025.07.0"
ARG ADIOS2_SPEC="~mpi~mgard~libcatalyst~bzip2~png"

RUN apt update && \
    DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
    bzip2 \
    ca-certificates \
    curl \
    file \
    g++-12 \
    gfortran-12 \
    git \
    gzip \
    lsb-release \
    patch \
    python3 \
    tar \
    unzip \
    xz-utils \
    zstd \
    \
    && \
    apt clean -y && \
    rm -rf /var/lib/apt/lists/*

# Clone and patch spack
WORKDIR /
RUN if ! [ -d /spack ]; then \
      git clone --depth 1 --single-branch --branch ${SPACK_VERSION} https://github.com/spack/spack; \
    else \
      git fetch --all && git checkout -t origin/${SPACK_VERSION}; \
    fi && \
    mkdir -p /root/.spack

COPY packages.yaml /root/.spack/packages.yaml

# Install base specs
RUN . /spack/share/spack/setup-env.sh && \
    spack repo update --branch "releases/v2025.07" builtin && \
    spack mirror add binaries "${SPACK_MIRROR}" && \
    spack config add "packages:all:target:[haswell]" && \
    spack config add "config:checksum:false" && \
    spack config add "config:build_jobs:$(nproc)" && \
    spack config add "concretizer:reuse:true"

RUN . /spack/share/spack/setup-env.sh && \
    spack install \
      -j$(nproc) \
      --include-build-deps \
      --no-check-signature \
      --fail-fast \
      --only dependencies \
      adios2@master ${ADIOS2_SPEC} cmake%gcc && \
      spack clean -a && \
    echo "source /spack/share/spack/setup-env.sh" >> ~/.bash_profile

ENV ADIOS2_SPEC="${ADIOS2_SPEC}"

ENTRYPOINT []
CMD ["bash", "--login"]
