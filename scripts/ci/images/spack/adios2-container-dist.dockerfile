FROM  ghcr.io/ornladios/adios2/adios2-deps

RUN echo ${ADIOS2_SPEC}

# Install from CI source
ARG ci_source_dir=ci-source
COPY ${ci_source_dir} /opt/adios2/source
RUN . /spack/share/spack/setup-env.sh && \
    spack dev-build \
        -j$(grep -c '^processor' /proc/cpuinfo) \
        -d /opt/adios2/source \
        --no-checksum \
        --skip-patch \
        --reuse \
        adios2@master ${ADIOS2_SPEC} ^gcc && \
    spack uninstall --all --yes-to-all gcc && \
    spack clean -a

RUN . /spack/share/spack/setup-env.sh && \
    spack config add "concretizer:unify:false" && \
    spack env create --without-view adios2 && \
    spack -e adios2 add $(spack find --format "/{hash}") && \
    spack -e adios2 install -v && \
    rm -rf /root/.spack && \
    spack env activate adios2 && \
    spack env deactivate && \
    echo "source /spack/share/spack/setup-env.sh" >> ~/.bash_profile && \
    echo "spack load adios2" >> ~/.bash_profile

ENTRYPOINT []
CMD ["bash", "--login"]
