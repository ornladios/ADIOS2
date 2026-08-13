#!/bin/bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

set -x
set -e

spack install --add -v tau ~fortran ~papi ~pdt ~otf2 ~elf
