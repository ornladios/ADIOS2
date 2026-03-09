#!/bin/bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

set -x
set -e

# shellcheck disable=SC1091
source "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"/setup.sh

# Fail if is not set
install_dir="${install_dir:?}"

"${install_dir}"/bin/adios2_hello_helloWorld
