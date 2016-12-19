# Makefile for testing purposes, will build libadios.a 
# Created on: Oct 4, 2016
#     Author: wfg
     
#SYSTEM DIRECTORIES, USER MUST MODIFY THESE VALUES
SYS_BIN:=/usr/bin
SYS_LIB:=/usr/lib/x86_64-linux-gnu
LOCAL_LIB:=/usr/local/lib

#COMPILERS AND LIBRARY LOCATIONS
CC:=$(SYS_BIN)/g++
AR:=$(SYS_BIN)/ar
MPICC:=$(SYS_BIN)/mpic++
LIBS:= -L$(SYS_LIB) -L$(LOCAL_LIB)

CFLAGS:=-c -Wall -O0 -g -Wpedantic -Woverloaded-virtual -std=c++14
ARFLAGS:=rcs

#ADIOS 
HFiles:=$(shell find ./include -type f -name "*.h")
CPPFiles:=$(shell find ./src -type f -name "*.cpp")
INC:=-I./include
VPATH = ./src ./src/core ./src/functions ./src/transform ./src/transport

#SEPARATE EXTERNAL LIBRARIES HANDLING in Makefile.libs
export $(HFiles) $(CPPFiles) $(CFLAGS) $(LIBS)
include Makefile.libs

OBJMPI:=$(patsubst %.cpp, ./bin/mpi/%.o, $(notdir $(CPPFiles)) )
OBJMPI:=$(patsubst ./bin/mpi/mpidummy.o, ,$(OBJMPI) )  #remove mpidummy from compilation

OBJNoMPI:=$(patsubst %.cpp, ./bin/nompi/%.o, $(notdir $(CPPFiles)) )

.PHONY: all clean mpi nompi

all: mpi nompi

mpi: $(HFiles) $(OBJMPI)
	@( mkdir -p ./lib );
	$(AR) $(ARFLAGS) ./lib/libadios.a $(OBJMPI)
	@echo "Finished building MPI library lib/libadios.a";
	@echo
    
./bin/mpi/%.o: %.cpp $(HFiles)
	@( mkdir -p ./bin/mpi );
	$(MPICC) $(CFLAGS) -DHAVE_MPI $(INC) -o $@ $<

nompi: $(HFiles) $(OBJNoMPI)
	@( mkdir -p ./lib );
	$(AR) $(ARFLAGS) ./lib/libadios_nompi.a $(OBJNoMPI)
	@echo "Finished building noMPI library lib/libadios_nompi.a";
	@echo

./bin/nompi/%.o: %.cpp $(HFiles)
	@( mkdir -p ./bin/nompi );
	$(CC) $(CFLAGS) $(INC) -o $@ $<
	
clean:
	rm ./bin/mpi/*.o ./lib/libadios.a ./bin/nompi/*.o ./lib/libadios_nompi.a	
