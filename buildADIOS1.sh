#!/bin/bash


# buildTest.sh for ADIOS1 Writer example
# Created on: Mar 27, 2017
#     Author: pnb
# Recommended: do a "make clean" the first time 

MPICOMPILER=mpic++
ADIOS1_DIR=/opt/adios/1.11

if [ "$(uname)" == "Darwin" ]; then
	CCOMPILER=clang++
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
	CCOMPILER=g++
fi
echo "######################################################################################"
echo "Start building ADIOS ./lib/libadios.a ./libadios_nompi.a" 
echo "######################################################################################"
echo
make -j4 HAVE_ADIOS1=yes ADIOS1_DIR=${ADIOS1_DIR} CC=$CCOMPILER MPICC=$MPICOMPILER       #build the ./lib/libadios.a and ./libadios_nompi.a
echo
echo "#################################################################"
echo "Building ADIOS1Writer example"
echo "#################################################################"
make -j4 -C ./examples/hello/adios1Writer CC=$CCOMPILER MPICC=$MPICOMPILER mpi
#make -j4 -C ./examples/hello/adios1Reader CC=$CCOMPILER MPICC=$MPICOMPILER mpi
echo
echo
echo
echo "#################################################################"
echo "Running the MPI example"
echo "#################################################################"
echo
echo
echo "#################################################################"
echo "ADIOS1 writer"
echo "#################################################################"
mpirun -np 4 ./examples/hello/adios1Writer/helloADIOS1Writer.exe
echo "DONE...check for myDoubles.bp directory"

echo "#################################################################"
echo "ADIOS1 reader..not ready yet"
echo "#################################################################"
#./examples/hello/adios1Writer/helloADIOS1Reader.exe
echo
echo
echo "#################################################################"
echo "To run mpi version with 4 mpi processes: "
echo "mpirun -n 4 ./examples/hello/bpWriter/helloBPWriter.exe"
echo "mpirun -n 4 ./examples/hello/bpReader/helloBPReader.exe"
echo "END"
echo "################################################################"
