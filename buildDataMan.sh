#!/bin/bash


# buildTest.sh for Dataman example
# Created on: Feb 9, 2017
#     Author: wfg

DATAMAN_LOCATION=/home/wfg/Applications/DataMan
echo "######################################################################################"
echo "Start building ADIOS ./lib/libadios.a ./libadios_nompi.a with DataMan library" 
echo "######################################################################################"
echo
make HAVE_DATAMAN=yes DATAMAN_LOC=$DATAMAN_LOCATION       #build the ./lib/libadios.a and ./libadios_nompi.a
echo
echo "#################################################################"
echo "Building Dataman Reader and Writer examples"
echo "#################################################################"
make -C ./examples/hello/datamanWriter HAVE_DATAMAN=yes DATAMAN_LOC=$DATAMAN_LOCATION
echo
make -C ./examples/hello/datamanReader HAVE_DATAMAN=yes DATAMAN_LOC=$DATAMAN_LOCATION

echo
echo
echo "#################################################################"
echo "Running nompi.exe examples"
echo "#################################################################"
echo
echo
echo "#################################################################"
echo "DataMan writer"
echo "#################################################################"
./examples/hello/datamanWriter/helloDataManWriter_nompi.exe

echo "#################################################################"
echo "DataMan reader"
echo "#################################################################"
./examples/hello/datamanReader/helloDataManReader_nompi.exe

echo
echo
echo "#################################################################"
echo "To run mpi version with 4 mpi processes: "
echo "mpirun -n 4 ./examples/hello/datamanWriter/helloDatamanWriter.exe"
echo "mpirun -n 4 ./examples/hello/datamanReader/helloDatamanReader.exe"
echo "END"
echo "################################################################"
