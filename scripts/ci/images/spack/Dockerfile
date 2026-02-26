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
        adios2@master && \
    spack clean -a

RUN . /spack/share/spack/setup-env.sh && \
    spack config add "concretizer:unify:false" && \
    spack env create --without-view adios2 && \
    spack -e adios2 add $(spack find --deps --format "/{hash}" adios2@master) && \
    spack -e adios2 install -v && \
    spack clean -a

RUN . /spack/share/spack/setup-env.sh && \
    spack env activate adios2 && \
    spack env deactivate && \
    echo "source /spack/share/spack/setup-env.sh" >> ~/.bash_profile && \
    echo "spack env activate adios2" >> ~/.bash_profile

ENTRYPOINT []
CMD ["bash", "--login"]
