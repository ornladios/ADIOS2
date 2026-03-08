#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

echo "---------- Begin ENV ----------"
env | sort
echo "----------  End ENV  ----------"

# If a source dir is given, then change to it.  Otherwise assume we're already
# in it
if [ -n "${SOURCE_DIR}" ]
then
  cd "${SOURCE_DIR}" || exit
fi

exec black --check --diff .
