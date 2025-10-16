#!/bin/bash

update-alternatives --set gcc "$(update-alternatives --list gcc | grep gcc-12)"
update-alternatives --set g++ "$(update-alternatives --list g++ | grep g++-12)"
update-alternatives --set gfortran "$(update-alternatives --list gfortran | grep gfortran-12)"

spack env activate "adios2-ci-${GH_YML_MATRIX_PARALLEL}"
