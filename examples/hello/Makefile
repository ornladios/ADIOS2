# Makefile for testing purposes, will build libadios.a 
# Created on: Oct 4, 2016
#     Author: wfg

     
TOOL_DIR=/usr/bin

CC=$(TOOL_DIR)/g++ # Compiling with mpicc for now
MPICC=$(TOOL_DIR)/mpic++
AR=$(TOOL_DIR)/ar

#ADIOS LOCATION
ADIOS_DIR=/home/wfg/workspace/ADIOSPP
ADIOS_LIB=$(ADIOS_DIR)/lib/libadios.a
ADIOS_NOMPI_LIB=$(ADIOS_DIR)/lib/libadios_nompi.a

ADIOS_INCLUDE=-I$(ADIOS_DIR)/include
ADIOS_MPI_HFiles=$(shell find $(ADIOS_DIR)/include/mpi/ -type f -name "*MPI.h")
ADIOS_NoMPI_HFiles=$(shell find $(ADIOS_DIR)/include/nompi/ -type f -name "*NoMPI.h")
ADIOS_HFiles=$(MPI_HFiles) $(NoMPI_HFiles)


#FLAGS
CFLAGS=-Wall -O0 -g -Wpedantic -std=c++11

all: mpi

mpi: $(ADIOS_LIB) $(ADIOS_HFiles)
	$(MPICC) $(CFLAGS) $(ADIOS_INCLUDE) -DHAVE_MPI helloADIOS.cpp -o helloADIOS $(ADIOS_LIB)
	
nompi: $(ADIOS_NOMPI_LIB) $(NoMPI_HFiles)
	$(CC) $(CFLAGS) $(ADIOS_INCLUDE) helloADIOS_nompi.cpp -o helloADIOS_nompi $(ADIOS_NOMPI_LIB)

clean:
	rm helloADIOS; rm helloADIOS_nompi
     
