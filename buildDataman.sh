#!/bin/bash


# buildTest.sh for Dataman example
# Created on: Feb 9, 2017
#     Author: wfg


echo "#################################################################"
echo "Start building ADIOS ./lib/libadios.a ./libadios_nompi.a"
make       #build the ./lib/libadios.a and ./libadios_nompi.a
echo "#################################################################"
echo

echo
echo "#################################################################"
echo "Building Dataman example"
echo "#################################################################"
make -C ./examples/hello/dataman

echo
echo
echo "#################################################################"
echo "Running helloDataman_nompi.exe example"
echo "#################################################################"
./examples/hello/dataman/helloDataMan_nompi.exe

echo
echo
echo "#################################################################"
echo "To run mpi version with 4 mpi processes: "
echo "mpirun -n 4 ./examples/hello/dataman/helloDataman.exe"
echo "END"
echo "################################################################"
