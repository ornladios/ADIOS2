#!/bin/bash
cat << EOF >> /etc/profile
set +u
source /opt/rh/devtoolset-7/enable

export C_INCLUDE_DIRS="/opt/rh/devtoolset-7/root/usr/include/c++/7:${C_INCLUDE_DIRS}"
export CPP_INCLUDE_DIRS="/opt/rh/devtoolset-7/root/usr/include/c++/7:${CPP_INCLUDE_DIRS}"
set -u
EOF
