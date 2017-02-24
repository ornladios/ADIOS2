# Makefile for testing purposes, will build libadios.a 
# Created on: Oct 4, 2016
#     Author: wfg


BASE_NAME=helloDataManReader

CC=g++-6 # Compiling with defaults
MPICC=mpic++
INC=
LIB=
LIB_NOMPI=

#ADIOS
ADIOS_DIR=../../..
LIB+= $(ADIOS_DIR)/lib/libadios.a
LIB_NOMPI+= $(ADIOS_DIR)/lib/libadios_nompi.a
INC+= -I$(ADIOS_DIR)/include

#DATAMAN
DATAMAN_LOC=/Users/w4g/Dropbox/lib/DataMan
INC+= -I$(DATAMAN_LOC)/include
LIB+= -L$(DATAMAN_LOC)/lib -ldataman # -lcacheman -lstreamman -lzmqman
LIB_NOMPI+= -L$(DATAMAN_LOC)/lib -ldataman

#COMPILER FLAGS
CFLAGS=-Wall -O0 -g -Wpedantic -std=c++11

#SYSTEM LIBS
LIB+= -lpthread
LIB_NOMPI+= -ldl -lpthread


all: mpi nompi

mpi: $(ADIOS_LIB) $(ADIOS_HFiles)
	$(MPICC) $(CFLAGS) $(INC) -DHAVE_MPI $(BASE_NAME).cpp -o $(BASE_NAME).exe $(LIB)
	
nompi: $(ADIOS_NOMPI_LIB) $(NoMPI_HFiles)
	$(CC) $(CFLAGS) $(INC) $(BASE_NAME)_nompi.cpp -o $(BASE_NAME)_nompi.exe $(LIB_NOMPI)

clean:
	rm *.exe;
     
