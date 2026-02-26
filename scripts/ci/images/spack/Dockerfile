FROM ghcr.io/ornladios/adios2:ci-spack-ubuntu22.04-base

# Install from CI source
ARG ci_source_dir=ci-source
COPY ${ci_source_dir} /opt/adios2/source

RUN . /spack/share/spack/setup-env.sh && \
    spack dev-build \
        -j$(nproc) \
        -d /opt/adios2/source \
        --no-checksum \
        --reuse \
        --skip-patch \
        adios2@master~mpi && \
    spack clean -a

RUN . /spack/share/spack/setup-env.sh && \
    echo "source /spack/share/spack/setup-env.sh" >> ~/.bash_profile && \
    echo "spack load adios2" >> ~/.bash_profile

ENTRYPOINT []
CMD ["bash", "--login"]
