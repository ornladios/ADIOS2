# vim: ft=dockerfile:
FROM ornladios/adios2:ci-x86_64-power8-ubuntu2004-base

# Install packages of XL compilers
COPY IBM_XL_C_CPP_*_LINUX_COMMUNITY.tar.gz /tmp
COPY IBM_XL_FORTRAN_*_LINUX_COMMUNITY.tar.gz /tmp
# Upgrades of XL compilers
COPY IBM_XL_C_CPP_*_LINUX.tar.gz /tmp
COPY IBM_XL_FORTRAN_*_LINUX.tar.gz /tmp
WORKDIR /tmp
RUN mkdir xlc && \
    cd xlc && \
    tar -xf ../IBM_XL_C_CPP_*_LINUX_COMMUNITY.tar.gz && \
    yes 1 | ./install && \
    cd .. && \
    mkdir xlf && \
    cd xlf && \
    tar -xf ../IBM_XL_FORTRAN_*_LINUX_COMMUNITY.tar.gz && \
    yes 1 | ./install && \
    cd .. && \
    mkdir xlc_p && \
    cd xlc_p && \
    tar -xf ../IBM_XL_C_CPP_*_LINUX.tar.gz && \
    yes 1 | ./install && \
    cd .. && \
    mkdir xlf_p && \
    cd xlf_p && \
    tar -xf ../IBM_XL_FORTRAN_*_LINUX.tar.gz && \
    yes 1 | ./install && \
    cd .. && \
    rm -rf /tmp/* && \
    yum -y clean all
ENV CC=/usr/bin/xlc \
    CXX=/usr/bin/xlc++ \
    FC=/usr/bin/xlf
