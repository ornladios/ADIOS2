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
echo "Building BPWriter example"
echo "#################################################################"
make -j4 -C ./examples/hello/bpWriter CC=$CCOMPILER MPICC=$MPICOMPILER
#make -j4 -C ./examples/hello/bpReader CC=$CCOMPILER MPICC=$MPICOMPILER
echo
echo
echo
echo "#################################################################"
echo "Running nompi.exe example"
echo "#################################################################"
echo
echo
echo "#################################################################"
echo "BP writer"
echo "#################################################################"
./examples/hello/bpWriter/helloBPWriter_nompi.exe
echo "DONE...check for myDoubles_nompi.bp directory"

echo "#################################################################"
echo "BP reader..not ready yet"
echo "#################################################################"
#./examples/hello/bpWriter/helloBPReader.exe
echo
echo
echo "#################################################################"
echo "To run mpi version with 4 mpi processes: "
echo "mpirun -n 4 ./examples/hello/bpWriter/helloBPWriter.exe"
echo "mpirun -n 4 ./examples/hello/bpReader/helloBPReader.exe"
echo "END"
echo "################################################################"
