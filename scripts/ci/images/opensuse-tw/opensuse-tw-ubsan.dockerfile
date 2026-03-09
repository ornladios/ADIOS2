# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

FROM ghcr.io/ornladios/adios2:ci-opensuse-tw-sanitizer-base

# Install core dev packages
RUN zypper install -y --no-recommends libubsan1 libasan8 hdf5-devel zfp-devel && \
    zypper clean --all
