#!/bin/bash


# buildTest.sh for vis engine build and run test
# Created on: Feb 9, 2017
#     Author: wfg


echo "#################################################################"
echo "Start building ADIOS ./lib/libadios.a ./libadios_nompi.a"
make       #build the ./lib/libadios.a and ./libadios_nompi.a
echo "#################################################################"
echo

echo
echo "#################################################################"
echo "Building vis example"
echo "#################################################################"
make -C ./examples/hello/vis

echo
echo
echo "#################################################################"
echo "Running vis nompi example"
echo "#################################################################"
./examples/hello/vis/helloVis_nompi

echo
echo
echo "#################################################################"
echo "To run mpi version with 4 mpi processes: "
echo "mpirun -n 4 ./examples/hello/vis/helloVis_mpi"
echo "END"
echo "################################################################"
