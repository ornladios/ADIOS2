#!/bin/bash

set -x
set -e

source $(dirname $(readlink -f ${BASH_SOURCE}))/setup.sh

cmake --install ${build_dir}
