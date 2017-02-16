# Makefile for testing purposes, will build writer_mpi (make or make mpi) or writer_nompi (make nompi) 
# Created on: Feb 13, 2017
#     Author: pnorbert

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

all: writer reader 

writer reader: $(ADIOS_LIB) $(ADIOS_HFiles)
	    $(MPICC) $(CFLAGS) $(ADIOS_INCLUDE) -DHAVE_MPI $@.cpp -o $@ $(ADIOS_LIB) $(LDFLAGS) -lpthread  \
	
clean:
	rm -f writer reader
     
