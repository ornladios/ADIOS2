#!/bin/sh

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

if command -v apt-get >/dev/null
then
  apt-get update
  apt-get install -y sudo
  apt-get clean
elif command -v yum >/dev/null
then
  yum install -y sudo
  yum clean all
fi 

useradd -m -s /bin/bash adios
echo "adios ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/adios
