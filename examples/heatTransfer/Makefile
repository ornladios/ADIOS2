# Makefile for testing purposes, will build writer_mpi (make or make mpi) or writer_nompi (make nompi) 
# Created on: Feb 13, 2017
#     Author: pnorbert

#COMPILERS
CC=g++ 
MPICC=mpic++

#ADIOS LOCATION
ADIOS_DIR=../..
ADIOS_INCLUDE=-I$(ADIOS_DIR)/include
ADIOS_LIB=$(ADIOS_DIR)/lib/libadios.a
ADIOS_NOMPI_LIB=$(ADIOS_DIR)/lib/libadios_nompi.a

#FLAGS
CFLAGS=-Wall -Wpedantic -Woverloaded-virtual -std=c++11 -O0 -g
LDFLAGS=

default: 
	@echo "Make targets: ascii hdf5_a hdf5_b phdf5 adios1 adios2"
	@echo "  ascii:    build example with text output(IO_ascii.cpp)"
	@echo "  hdf5_a:   build example with HDF5, separate timesteps (IO_hdf5_a.cpp)"
	@echo "  hdf5_b:   build example with HDF5, combined timesteps (IO_hdf5_b.cpp)"
	@echo "  phdf5:    build example with Parallel HDF5  (IO_phdf5.cpp)"
	@echo "  adios1:   build example with ADIOS 1.x  (IO_adios1.cpp)"
	@echo "  adios2:   build example with ADIOS2  (IO_adios2.cpp)"


all: default
help: default


ascii: main.cpp HeatTransfer.cpp Settings.cpp IO_ascii.cpp 
	    $(MPICC) $(CFLAGS) $(ADIOS_INCLUDE) -DHAVE_MPI -o heatTransfer_ascii $^ $(ADIOS_LIB) $(LDFLAGS) -lpthread  

adios2: main.cpp HeatTransfer.cpp Settings.cpp IO_adios2.cpp 
	    $(MPICC) $(CFLAGS) $(ADIOS_INCLUDE) -DHAVE_MPI -o heatTransfer_adios2 $^ $(ADIOS_LIB) $(LDFLAGS) -lpthread  
	
clean:
	rm -f heatTransfer_ascii heatTransfer_adios2
     
