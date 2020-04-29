##############################################################################
# This is a base image to be used for multi-arch emulation on CI.  To setup
# your system to correctly support the emulation you'll need to register
# the qemu static user binaries with your kernel's binfmt_misc infrastructure:
#
#   docker run --rm --privileged multiarch/qemu-user-static:register --reset
#
# To build this image you want to set the following build args:
#   TARGET_ARCH_SYSTEM=ppc64le - The CPU architecture of the target system.
#   TARGET_ARCH_DOCKER=ppc64le - The CPU architecture if the target system as
#                                known by dockerhub. Most of the time this will
#                                be the same as TARGET_ARCH_SYSTEM but in a few
#                                cases they're different, e.g. for 64-bit ARM
#                                you would set TARGET_ARCH_SYSTEM=aarch64 and
#                                TARGET_ARCH_DOCKER=arm64v8.
#   TARGET_CPU=power8          - The specific CPU model of the target system
#                                to emulate.  For example, on ppc64le you
#                                could set it to power8 or power9 (among others)
#                                or on aarch64 you could set it to cortex-a72.
################################################################################
ARG TARGET_ARCH_SYSTEM=ppc64le
ARG TARGET_ARCH_DOCKER=ppc64le

########################################
# QEMU image with emulation binaries
########################################
FROM multiarch/qemu-user-static:x86_64-${TARGET_ARCH_SYSTEM} AS qemu

########################################
# Base image
########################################
FROM ${TARGET_ARCH_DOCKER}/centos:centos7

########################################
# Grab a fully static node.js binary
# to run various CI actions
########################################
COPY --from=ornladios/adios2:node12-static /node /x86_64/bin/node

########################################
# Build up a minimal busybox shell
# environment to run scripts on the host
# CPU
########################################
COPY --from=busybox /bin/busybox /x86_64/bin/busybox
SHELL ["/x86_64/bin/busybox", "sh", "-c"]
RUN /x86_64/bin/busybox --install /x86_64/bin

########################################
# Get rid of the busybox uname because
# it runs in the host CPU and we need
# CI scripts to instead use
# /usr/bin/uname from the emulated CPU
########################################
RUN /x86_64/bin/rm -f /x86_64/bin/uname

########################################
# Put busybox in the path to allow job
# steps to use either the native host
# shell or the emulated CPU shell
########################################
RUN /x86_64/bin/ln -s /x86_64/bin/busybox /usr/local/bin/busybox

########################################
# Put the busybox tail command in the
# path to be used as the default
# entrypoint in CI
########################################
RUN /x86_64/bin/ln -s /x86_64/bin/tail /usr/local/bin/tail
 
########################################
# QEMU CPU emulation binary
########################################
ARG TARGET_ARCH_SYSTEM=ppc64le
COPY --from=qemu \
        /usr/bin/qemu-${TARGET_ARCH_SYSTEM}-static \
        /x86_64/bin/qemu-${TARGET_ARCH_SYSTEM}-static

########################################
# Hard link the emulation binary to the
# expected location used by the default
# registration scripts for ease of local
# development.
########################################
ARG TARGET_ARCH_SYSTEM=ppc64le
RUN /x86_64/bin/ln \
        /x86_64/bin/qemu-${TARGET_ARCH_SYSTEM}-static \
        /usr/bin/qemu-${TARGET_ARCH_SYSTEM}-static

########################################
# Scripts for emu registration
########################################
COPY qemu-binfmt-conf.sh /x86_64/bin/qemu-binfmt-conf.sh
COPY register.sh /x86_64/bin/register

########################################
# Set the specific CPU
########################################
ARG TARGET_CPU=power8
ENV QEMU_CPU=${TARGET_CPU}

########################################
# Setup the entrypoint using busybox for
# the host cpu
########################################
COPY entrypoint.sh /x86_64/bin/entrypoint
ENTRYPOINT ["/x86_64/bin/entrypoint"]
CMD ["/x86_64/bin/sh"]

########################################
# Reset the shell for subsequent image
# building
########################################
SHELL ["/bin/sh", "-c"]
