ARG DISTRO=centos7
FROM spack/${DISTRO}

COPY adios2-ci-env.yml /root/adios2-ci-env.yml
RUN spack env create adios2-ci /root/adios2-ci-env.yml && \
    spack env activate adios2-ci && \
    spack install -v -j$(grep -c '^processor' /proc/cpuinfo) && \
    spack clean -a
RUN rm -f /root/adios2-ci-env.yml
