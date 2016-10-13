# Makefile for testing purposes, will build libadios.a 
# Created on: Oct 4, 2016
#     Author: wfg

     
TOOL_DIR=/usr/bin

CC=$(TOOL_DIR)/g++ # Compiling with mpicc for now
#CC=$(TOOL_DIR)/clang # Compiling with mpicc for now

MPICC=$(TOOL_DIR)/mpic++

AR=$(TOOL_DIR)/ar

#FLAGS
CFLAGS=-c -Wall -O0 -g -Wpedantic -std=c++11


#INCLUDE FILES
ADIOS_INC=-I./include

INCLUDE=$(ADIOS_INC)

#Build Header Dependencies, if one changes it will rebuild
MPI_HFiles=$(shell find ./include/mpi -type f -name "*MPI.h")
NoMPI_HFiles=$(shell find ./include/nompi -type f -name "*NoMPI.h")
Local_HFiles=$(shell find ./include -type f -name "*.h")
HFiles=$(MPI_HFiles) $(NoMPI_HFiles) $(Local_HFiles)

#Source *.cpp Files and Object Files
MPI_CPPFiles=$(shell find ./src/mpi -type f -name "*MPI.cpp")
MPI_ObjFiles=$(patsubst ./src/mpi/transport/%.cpp, ./bin/%.o, $(MPI_CPPFiles))

NoMPI_CPPFiles=$(shell find ./src/nompi -type f -name "*NoMPI.cpp")
NoMPI_ObjFiles=$(patsubst ./src/nompi/transport/%.cpp, ./bin/%.o, $(NoMPI_CPPFiles))

Common_CPPFiles=$(wildcard ./src/*.cpp)
Common_ObjFiles=$(patsubst ./src/%.cpp, ./bin/%.o, $(Common_CPPFiles))
 

ObjFiles=$(MPI_ObjFiles) $(NoMPI_ObjFiles)


#Build all MPI and noMPI
all: $(MPI_ObjFiles) $(NoMPI_ObjFiles) $(Common_ObjFiles) $(HFiles)
	$(AR) rcs ./lib/libadios.a $(MPI_ObjFiles) $(NoMPI_ObjFiles) ./bin/ADIOS.o ./bin/ADIOSFunctions.o ./bin/CGroup.o

#MPI build    
mpi: $(MPI_ObjFiles) $(Common_ObjFiles) $(MPI_HFiles) $(Local_HFiles)
	$(AR) rcs ./lib/libadios.a $(MPI_ObjFiles) ./bin/ADIOS.o ./bin/ADIOSFunctions.o ./bin/CGroup.o
	
./bin/%.o: ./src/mpi/transport/%.cpp
	$(MPICC) $(CFLAGS) -DHAVE_MPI $(INCLUDE) -o $@ $< 
	
./bin/%.o: ./src/%.cpp
	$(MPICC) $(CFLAGS) -DHAVE_MPI $(INCLUDE) -o $@ $<
    
#NoMPI build    
nompi: $(NoMPI_ObjFiles) $(Common_ObjFiles) $(NoMPI_HFiles) $(Local_HFiles)
	$(AR) rcs ./lib/libadios_nompi.a $(NoMPI_ObjFiles) $(Common_ObjFiles)
	
./bin/%.o: ./src/nompi/transport/%.cpp $(NoMPI_HFiles) $(Local_HFiles)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $<
	    
clean:
	rm ./bin/*.o ./lib/libadios.a ./lib/libadios_nompi.a
	
