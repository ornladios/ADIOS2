#!/usr/bin/env bash

source /opt/spack/share/spack/setup-env.sh

# External CUDA is erroneously treated as a module, in no CUDA builds this is no-op
find $SPACK_ROOT/share/spack/modules -type f -exec sed -i '/module load cuda/d' {} \;

echo 'export LD_LIBRARY_PATH=/usr/local/cuda/compat/:$LD_LIBRARY_PATH' >> /etc/profile.d/zz-adios2-ci-env.sh
