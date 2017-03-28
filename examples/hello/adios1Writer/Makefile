# Makefile for testing purposes, will build helloADIOS1Writer (make or make mpi) or helloADIOS1Writer_nompi (make nompi) 
# Created on: Mar 27, 2017
#     Author: pnb

BASE_NAME=helloADIOS1Writer

#COMPILERS
CC=g++
MPICC=mpic++

#ADIOS 1.x  LOCATION
ADIOS1_DIR=/opt/adios/1.11
ADIOS1_LIB=$(shell ${ADIOS1_DIR}/bin/adios_config -l)
ADIOS1_NOMPI_LIB=$(shell ${ADIOS1_DIR}/bin/adios_config -ls)

#ADIOS 2.x LOCATION
ADIOS_DIR=../../..
ADIOS_INCLUDE=-I$(ADIOS_DIR)/include
ADIOS_LIB=$(ADIOS_DIR)/lib/libadios.a
ADIOS_NOMPI_LIB=$(ADIOS_DIR)/lib/libadios_nompi.a

#FLAGS
CFLAGS=-Wall -Wpedantic -std=c++11 -O0 -g
LDFLAGS=

all: mpi nompi

mpi: $(ADIOS_LIB) $(ADIOS_HFiles)
	$(MPICC) $(CFLAGS) $(ADIOS_INCLUDE) $(BASE_NAME).cpp -o $(BASE_NAME).exe $(ADIOS_LIB) $(ADIOS1_LIB) $(LDFLAGS) -lpthread
	
nompi: $(ADIOS_NOMPI_LIB) $(NoMPI_HFiles)
	$(CC) $(CFLAGS) $(ADIOS_INCLUDE) -DADIOS_NOMPI $(BASE_NAME)_nompi.cpp -o $(BASE_NAME)_nompi.exe $(ADIOS_NOMPI_LIB) $(ADIOS1_NOMPI_LIB) $(LDFLAGS) -lpthread

clean:
	rm *.exe
     
