#!/bin/bash

set -x
set -e

sudo /opt/spack/bin/spack install parallel-netcdf
sudo /opt/spack/bin/spack -e adios2 add parallel-netcdf
sudo /opt/spack/bin/spack -e adios2 install -v
