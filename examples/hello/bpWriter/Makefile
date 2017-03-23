# Makefile for testing purposes, will build helloWriter_OOP_mpi (make or make mpi) or helloWriter_OOP_nompi (make nompi) 
# Created on: Oct 4, 2016
#     Author: wfg

BASE_NAME=helloBPWriter

#COMPILERS
CC=g++
MPICC=mpic++

#ADIOS LOCATION
ADIOS_DIR=../../..
ADIOS_INCLUDE=-I$(ADIOS_DIR)/include
ADIOS_LIB=$(ADIOS_DIR)/lib/libadios.a
ADIOS_NOMPI_LIB=$(ADIOS_DIR)/lib/libadios_nompi.a

#FLAGS
CFLAGS=-Wall -Wpedantic -std=c++11 -O0 -g
LDFLAGS=

all: mpi nompi

mpi: $(ADIOS_LIB) $(ADIOS_HFiles)
	$(MPICC) $(CFLAGS) $(ADIOS_INCLUDE) $(BASE_NAME).cpp -o $(BASE_NAME).exe $(ADIOS_LIB) $(LDFLAGS) -lpthread
	
nompi: $(ADIOS_NOMPI_LIB) $(NoMPI_HFiles)
	$(CC) $(CFLAGS) $(ADIOS_INCLUDE) -DADIOS_NOMPI $(BASE_NAME)_nompi.cpp -o $(BASE_NAME)_nompi.exe $(ADIOS_NOMPI_LIB) $(LDFLAGS) -lpthread

clean:
	rm *.exe
     