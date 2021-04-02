ARG baseos
FROM ornladios/adios2:spack-dependencies-${baseos}

# Install from CI source
ARG ci_source_dir=ci-source
COPY ci-source /opt/adios2/source
RUN spack dev-build \
        -j$(grep -c '^processor' /proc/cpuinfo) \
        -d /opt/adios2/source \
        --skip-patch \
        ${ADIOS2_SPACK_SPEC/adios2/adios2@develop} && \
    spack clean -a

# Build the environment
RUN spack env create adios2 && \
    spack -e adios2 add $(spack find --format "/{hash}") && \
    spack -e adios2 install -v && \
    rm -rf /root/.spack && \
    spack env activate adios2 && \
    spack env deactivate

# Initialize the first-use-of-environment stuff for the adios user
WORKDIR /home/adios
USER adios
RUN spack env activate adios2 && \
    spack env deactivate

ENTRYPOINT ["/usr/local/bin/spack-named-env", "adios2"]
CMD ["bash"]
