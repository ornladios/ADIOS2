#!/bin/bash

set -x
set -e

source $(dirname $(readlink -f ${BASH_SOURCE}))/setup.sh

cmake --build ${build_dir} -j8
