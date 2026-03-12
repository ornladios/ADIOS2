# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

ARG INSTALL_PREFIX=/opt/msan
ARG TOOLCHAIN_FILE=toolchain-msan.cmake
ARG CFLAGS="-fsanitize=memory -fsanitize-memory-track-origins"
ARG CXXFLAGS="-fsanitize=memory -fsanitize-memory-track-origins"
ARG LLVM_USE_SANITIZER=MemoryWithOrigins

FROM ghcr.io/ornladios/adios2:ci-opensuse-tw-full-stack-onbuild AS tmp-stage
FROM ghcr.io/ornladios/adios2:ci-opensuse-tw-sanitizer-base
COPY --from=tmp-stage /opt/msan/ /opt/msan/
