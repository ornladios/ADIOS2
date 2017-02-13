# Makefile for testing purposes, will build libadios.a 
# Created on: Oct 31, 2016
#     Author: pnorbert


BASE_NAME=solidfluid_write
     
TOOL_DIR=/usr/bin

CC=$(TOOL_DIR)/g++ # Compiling with mpicc for now
MPICC:=mpic++
AR=$(TOOL_DIR)/ar

#ADIOS LOCATION
ADIOS_DIR=../..
ADIOS_LIB=$(ADIOS_DIR)/lib/libadios.a
ADIOS_NOMPI_LIB=$(ADIOS_DIR)/lib/libadios_nompi.a

ADIOS_INCLUDE=-I$(ADIOS_DIR)/include


#FLAGS
CFLAGS=-Wall -O0 -g -Wpedantic -std=c++11

all: mpi

mpi: $(ADIOS_LIB) $(ADIOS_HFiles)
	$(MPICC) $(CFLAGS) $(ADIOS_INCLUDE) -DHAVE_MPI $(BASE_NAME).cpp -o $(BASE_NAME)_mpi $(ADIOS_LIB)
	
nompi: $(ADIOS_NOMPI_LIB) $(NoMPI_HFiles)
	$(CC) $(CFLAGS) $(ADIOS_INCLUDE) $(BASE_NAME).cpp -o $(BASE_NAME)_nompi $(ADIOS_NOMPI_LIB)

clean:
	rm -f *_mpi *_nompi *.o 
	rm -rf *.dSYM
     
