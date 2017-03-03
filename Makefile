# Makefile for testing purposes, will build libadios.a, libadios_nompi.a 
# Created on: Oct 4, 2016
#     Author: wfg
     
#DEFAULT COMPILERS IN PATH, LIBS will be modified in Makefile.libs
CC:=g++
AR:=ar
MPICC:=mpic++
LIBS:=

CFLAGS:=-c -Wall -Wpedantic -std=c++11 -O0 -g
ARFLAGS:=rcs

#ADIOS 
HFiles:=$(shell find ./include -type f -name "*.h")
CPPFiles:=$(shell find ./src -type f -name "*.cpp")
INC:=-I./include
VPATH = ./src ./src/core ./src/functions \
        ./src/engine/bp ./src/engine/dataman \
		./src/transport/file ./src/transport/wan \
		./src/capsule/heap ./src/capsule/shmem \
		./src/transform \
		./src/format

#SEPARATE EXTERNAL HEADERS AND LIBRARIES HANDLING in Makefile.libs, export variables 
export $(HFiles) $(CPPFiles) $(CFLAGS) $(INC) $(LIBS)  
include Makefile.libs

OBJMPI:=$(patsubst %.cpp, ./bin/mpi/%.o, $(notdir $(CPPFiles)) )
OBJMPI:=$(patsubst ./bin/mpi/mpidummy.o, ,$(OBJMPI) )  #remove mpidummy from compilation

OBJNoMPI:=$(patsubst %.cpp, ./bin/nompi/%.o, $(notdir $(CPPFiles)) )
OBJNoMPI:=$(patsubst ./bin/nompi/MPIFile.o, ,$(OBJNoMPI) )  #remove MPIFile from compilation (not supported in serial)

.PHONY: all clean mpi nompi

all: mpi nompi

mpi: $(HFiles) $(OBJMPI)
	@( mkdir -p ./lib );
	$(AR) $(ARFLAGS) ./lib/libadios.a $(OBJMPI)
	@echo "Finished building MPI library lib/libadios.a";
	@echo
    
./bin/mpi/%.o: %.cpp $(HFiles)
	@( mkdir -p ./bin/mpi );
	$(MPICC) $(CFLAGS) $(INC) -o $@ $<

nompi: $(HFiles) $(OBJNoMPI)
	@( mkdir -p ./lib );
	$(AR) $(ARFLAGS) ./lib/libadios_nompi.a $(OBJNoMPI)
	@echo "Finished building noMPI library lib/libadios_nompi.a";
	@echo

./bin/nompi/%.o: %.cpp $(HFiles)
	@( mkdir -p ./bin/nompi );
	$(CC) $(CFLAGS) $(INC) -DADIOS_NOMPI -o $@ $<
	
clean:
	rm ./bin/mpi/*.o ./lib/libadios.a ./bin/nompi/*.o ./lib/libadios_nompi.a	
