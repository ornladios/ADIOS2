#!/bin/bash

set -x
set -e

source $(dirname $(readlink -f ${BASH_SOURCE}))/setup.sh

mkdir -p ${build_dir}
cd ${build_dir}

cmake \
  -DCMAKE_INSTALL_PREFIX=${install_dir} \
  ${source_dir}
