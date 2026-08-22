# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

ARG baseos
FROM docker.io/ornladios/adios2:spack-dependencies-${baseos}

# spack-dependencies-ubuntu-bionic only ships Ubuntu Bionic's default GCC 7,
# which lacks stable std::filesystem support; install and register a newer
# GCC for spack to build adios2 with.
ARG GCC_VERSION=9
RUN apt-get update && \
    apt-get install -y software-properties-common && \
    add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    apt-get install -y gcc-${GCC_VERSION} g++-${GCC_VERSION} gfortran-${GCC_VERSION} && \
    rm -rf /var/lib/apt/lists/* && \
    spack compiler find /usr/bin

# Install from CI source
ARG ci_source_dir=ci-source
COPY ${ci_source_dir} /opt/adios2/source
RUN spack dev-build \
        -j$(grep -c '^processor' /proc/cpuinfo) \
        -d /opt/adios2/source \
        --skip-patch \
        --reuse \
        adios2@develop %gcc@${GCC_VERSION}: ~python && \
    spack clean -a

# Build the environment
RUN spack env create --without-view adios2 && \
    spack -e adios2 add $(spack find --format "/{hash}") && \
    spack -e adios2 install -v && \
    rm -rf /root/.spack && \
    spack env activate adios2 && \
    spack env deactivate && \
    spack -e adios2 env loads

# Initialize the first-use-of-environment stuff for the adios user
WORKDIR /home/adios
USER adios
RUN spack env activate adios2 && \
    spack env deactivate && \
    echo "source /opt/spack/share/spack/setup-env.sh" >> ~/.bash_profile && \
    echo "module use ${SPACK_ROOT}/share/spack/modules/linux-ubuntu18.04-haswell" >> ~/.bash_profile && \
    echo "source ${SPACK_ROOT}/var/spack/environments/adios2/loads" >> ~/.bash_profile

ENTRYPOINT []
CMD ["bash", "--login"]
