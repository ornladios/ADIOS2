#!/bin/bash

if [ -z "${CMAKE_PREFIX_PATH:-}" ]; then
    PREFIX_PATH=""
else
    PREFIX_PATH="${CMAKE_PREFIX_PATH}:"
fi

export CMAKE_PREFIX_PATH="${PREFIX_PATH}/opt/rocm/"

update-alternatives --set gcc "$(update-alternatives --list gcc | grep gcc-11)"
update-alternatives --set g++ "$(update-alternatives --list g++ | grep g++-11)"
update-alternatives --set gfortran "$(update-alternatives --list gfortran | grep gfortran-11)"

spack env activate "adios2-ci-${GH_YML_MATRIX_PARALLEL}"
