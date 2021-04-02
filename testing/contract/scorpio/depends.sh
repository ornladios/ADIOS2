#!/bin/bash

set -x
set -e

sudo /opt/spack/bin/spack install parallel-netcdf arch=x86_64
