# Makefile for testing purposes, will build helloWriter_OOP_mpi (make or make mpi) or helloWriter_OOP_nompi (make nompi) 
# Created on: Oct 4, 2016
#     Author: wfg

BASE_NAME=helloWriter_OOP

#COMPILERS
CC=g++ 
MPICC=mpic++

#ADIOS LOCATION
ADIOS_DIR=../../..
ADIOS_INCLUDE=-I$(ADIOS_DIR)/include
ADIOS_LIB=$(ADIOS_DIR)/lib/libadios.a
ADIOS_NOMPI_LIB=$(ADIOS_DIR)/lib/libadios_nompi.a

#FLAGS
CFLAGS=-Wall -Wpedantic -Woverloaded-virtual -std=c++11 -O0 -g
LDFLAGS=

all: mpi

mpi: $(ADIOS_LIB) $(ADIOS_HFiles)
	$(MPICC) $(CFLAGS) $(ADIOS_INCLUDE) -DHAVE_MPI $(BASE_NAME).cpp -o $(BASE_NAME)_mpi $(ADIOS_LIB) $(LDFLAGS) -lpthread
	
nompi: $(ADIOS_NOMPI_LIB) $(NoMPI_HFiles)
	$(CC) $(CFLAGS) $(ADIOS_INCLUDE) $(BASE_NAME).cpp -o $(BASE_NAME)_nompi $(ADIOS_NOMPI_LIB) $(LDFLAGS) -lpthread

clean:
	rm *_mpi; rm *_nompi
     