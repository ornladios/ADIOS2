# Makefile for testing purposes, will build libadios.a 
# Created on: Oct 4, 2016
#     Author: wfg


BASE_NAME=helloDataManWriter

CC=g++ # Compiling with defaults
MPICC=mpic++

#ADIOS LOCATION
ADIOS_DIR=../../..
ADIOS_LIB=$(ADIOS_DIR)/lib/libadios.a
ADIOS_NOMPI_LIB=$(ADIOS_DIR)/lib/libadios_nompi.a

ADIOS_INCLUDE=-I$(ADIOS_DIR)/include


#FLAGS
CFLAGS=-Wall -O0 -g -Wpedantic -std=c++11

all: mpi nompi

mpi: $(ADIOS_LIB) $(ADIOS_HFiles)
	$(MPICC) $(CFLAGS) $(ADIOS_INCLUDE) -DHAVE_MPI $(BASE_NAME).cpp -o $(BASE_NAME).exe $(ADIOS_LIB) -lpthread
	
nompi: $(ADIOS_NOMPI_LIB) $(NoMPI_HFiles)
	$(CC) $(CFLAGS) $(ADIOS_INCLUDE) $(BASE_NAME)_nompi.cpp -o $(BASE_NAME)_nompi.exe $(ADIOS_NOMPI_LIB) -lpthread

clean:
	rm *.exe;
     
