ARG baseos
FROM ornladios/adios2:spack-dependencies-${baseos}

# Install from CI source
COPY ci-source /root/ci-source
RUN spack dev-build \
        -j$(grep -c '^processor' /proc/cpuinfo) \
        -d /root/ci-source \
        --skip-patch \
        ${ADIOS2_SPACK_SPEC/adios2/adios2@develop} arch=x86_64 && \
    spack clean -a

# Build the environment
RUN spack env create adios2 && \
    spack -e adios2 add $(spack find --format "/{hash}") && \
    spack -e adios2 install && \
    rm -rf /root/.spack && \
    spack env activate adios2 && \
    spack env deactivate

# Cleanup
RUN rm -rf /root/ci-source 

# Initialize the first-use-of-environment stuff for the adios user
WORKDIR /home/adios
USER adios
RUN spack env activate adios2 && \
    spack env deactivate

ENTRYPOINT ["/usr/local/bin/spack-named-env", "adios2"]
CMD ["bash"]
