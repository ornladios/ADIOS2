#!/bin/bash

docker build -t "ornladios/adios2:ci-spack-el8-gcc10-mpich" \
             -f Dockerfile.ci-spack-el8-leaf \
             --build-arg COMPILER_IMG_BASE=gcc10 \
             --build-arg EXTRA_VARIANTS=+mpi \
             --build-arg MPI_FLAVOR=mpich \
             --build-arg COMPILER_SPACK_ID=gcc \
             .
