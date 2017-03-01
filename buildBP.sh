#!/bin/bash


# buildTest.sh for BP Writer example
# Created on: Feb 9, 2017
#     Author: wfg
# Recommended: do a "make clean" the first time 

MPICOMPILER=mpic++

if [ "$(uname)" == "Darwin" ]; then
	CCOMPILER=clang++
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
	CCOMPILER=g++
fi
echo "######################################################################################"
echo "Start building ADIOS ./lib/libadios.a ./libadios_nompi.a" 
echo "######################################################################################"
echo
make -j4 CC=$CCOMPILER MPICC=$MPICOMPILER       #build the ./lib/libadios.a and ./libadios_nompi.a
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
