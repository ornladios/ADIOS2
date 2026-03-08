#!/bin/bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

source_dir=$(readlink -f "${PWD}")/source
build_dir=$(readlink -f "${PWD}")/build
install_dir=$(readlink -f "${PWD}")/install
test_dir=$(readlink -f "${PWD}")/test

export source_dir
export build_dir
export install_dir
export test_dir

echo "source_dir  = \"${source_dir}\""
echo "build_dir   = \"${build_dir}\""
echo "install_dir = \"${install_dir}\""
echo "test_dir    = \"${test_dir}\""
