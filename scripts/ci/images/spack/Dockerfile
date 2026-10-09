# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0
FROM ghcr.io/ornladios/adios2:spack-dependencies-26.06

# Install from CI source
ARG ci_source_dir=ci-source
COPY ${ci_source_dir} /opt/adios2/source
RUN . /spack/share/spack/setup-env.sh && \
    spack dev-build \
        -j$(grep -c '^processor' /proc/cpuinfo) \
        -d /opt/adios2/source \
        --skip-patch \
        adios2@2~python && \
    spack clean -a

# Build the environment
RUN . /spack/share/spack/setup-env.sh && \
    spack env create adios2 && \
    spack -e adios2 config add "concretizer:unify:false" && \
    spack -e adios2 config add "modules:prefix_inspections:lib:[LD_LIBRARY_PATH]" && \
    spack -e adios2 config add "modules:prefix_inspections:lib64:[LD_LIBRARY_PATH]" && \
    spack -e adios2 add $(spack find --format "{name}/{hash}" adios2) && \
    spack -e adios2 install -v && \
    rm -rf /root/.spack && \
    spack env activate adios2 && \
    spack env deactivate

RUN echo "source /spack/share/spack/setup-env.sh" >> ~/.bash_profile && \
    echo "spack env activate adios2" >> ~/.bash_profile

ENTRYPOINT ["bash", "--login", "-c"]
CMD ["bash"]
