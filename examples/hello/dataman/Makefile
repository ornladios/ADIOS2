# Makefile for testing purposes, will build libadios.a 
# Created on: Oct 4, 2016
#     Author: wfg


BASE_NAME=helloDataMan_OOP
     
TOOL_DIR=/usr/bin

CC=g++ # Compiling with mpicc for now
MPICC=mpic++

#ADIOS LOCATION
ADIOS_DIR=../../..
ADIOS_LIB=$(ADIOS_DIR)/lib/libadios.a
ADIOS_NOMPI_LIB=$(ADIOS_DIR)/lib/libadios_nompi.a

ADIOS_INCLUDE=-I$(ADIOS_DIR)/include


#FLAGS
CFLAGS=-Wall -O0 -g -Wpedantic -std=c++11

all: mpi

mpi: $(ADIOS_LIB) $(ADIOS_HFiles)
	$(MPICC) $(CFLAGS) $(ADIOS_INCLUDE) -DHAVE_MPI $(BASE_NAME).cpp -o $(BASE_NAME)_mpi $(ADIOS_LIB) -lpthread
	
nompi: $(ADIOS_NOMPI_LIB) $(NoMPI_HFiles)
	$(CC) $(CFLAGS) $(ADIOS_INCLUDE) $(BASE_NAME).cpp -o $(BASE_NAME)_nompi $(ADIOS_NOMPI_LIB) -lpthread

clean:
	rm *_mpi; rm *_nompi
     
