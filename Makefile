# Makefile for testing purposes, will build libadios.a 
# Created on: Oct 4, 2016
#     Author: wfg
     
#SYSTEM DIRECTORIES, USER MUST MODIFY THESE VALUES
SYS_BIN:=/usr/bin
SYS_LIB:=/usr/lib/x86_64-linux-gnu
LOCAL_LIB:=/usr/local/lib

#COMPILERS
CC:=$(SYS_BIN)/g++
MPICC:=$(SYS_BIN)/mpic++
AR:=$(SYS_BIN)/ar

#FLAGS
CFLAGS:=-c -Wall -O0 -g -Wpedantic -std=c++11

#ALL FILES
HFiles:=$(shell find ./include -type f -name "*.h")
CPPFiles:=$(shell find ./src -type f -name "*.cpp")

#EXTERNAL LIBRARY PATHS

LIBS := -L$(SYS_LIB) -L$(LOCAL_LIB)

#TRANSPORT WITH EXTERNAL DEPENDENCIES
TRANSPORT_INC:=./include/transport
TRANSPORT_SRC:=./src/transport

ifeq ($(HAVE_NETCDF),yes)
    LIBS += -lnetcdf
else
    HFiles:=$(filter-out $(TRANSPORT_INC)/CNetCDF4.h,$(HFiles))
    CPPFiles:=$(filter-out $(TRANSPORT_SRC)/CNetCDF4.cpp,$(CPPFiles))     
endif

ifeq ($(HAVE_PHDF5),yes)
    LIBS += -lhdf5
else
    HFiles:=$(filter-out $(TRANSPORT_INC)/CPHDF5.h,$(HFiles))
    CPPFiles:=$(filter-out $(TRANSPORT_SRC)/CPHDF5.cpp,$(CPPFiles))     
endif


#TRANSFORM WITH EXTERNAL DEPENDENCIES
TRANSFORM_INC:=./include/transform
TRANSFORM_SRC:=./src/transform

ifeq ($(HAVE_BZIP2),yes)
    LIBS += -lbz2
else
    HFiles:=$(filter-out $(TRANSFORM_INC)/CSZIP.h,$(HFiles))
    CPPFiles:=$(filter-out $(TRANSFORM_SRC)/CSZIP.cpp,$(CPPFiles))     
endif

ifeq ($(HAVE_SZIP),yes)
    LIBS += -lsz
else
    HFiles:=$(filter-out $(TRANSFORM_INC)/CSZIP.h,$(HFiles))
    CPPFiles:=$(filter-out $(TRANSFORM_SRC)/CSZIP.cpp,$(CPPFiles))     
endif

ifeq ($(HAVE_ZLIB),yes)
    LIBS += -lz
else
    HFiles:=$(filter-out $(TRANSFORM_INC)/CZLIB.h,$(HFiles))
    CPPFiles:=$(filter-out $(TRANSFORM_SRC)/CZLIB.cpp,$(CPPFiles))     
endif




OBJFiles:=$(patsubst %.cpp, ./bin/%.o, $(notdir $(CPPFiles)) )


all:
	@echo $(HFiles);
	@echo $(CFLAGS);
	@echo $(LIBS);
	@echo $(CPPFiles);
	@echo $(OBJFiles)


##INCLUDE FILES
#ADIOS_INC=-I./include
#INCLUDE=$(ADIOS_INC)
#
#
#
#
##Build Header Dependencies, if one changes it will rebuild
#MPI_HFiles=$(shell find ./include/mpi -type f -name "*.h")
#NoMPI_HFiles=$(shell find ./include/nompi -type f -name "*NoMPI.h")
#Local_HFiles=$(shell find ./include -type f -name "*.h")
#HFiles=$(MPI_HFiles) $(NoMPI_HFiles) $(Local_HFiles)
#
##Source *.cpp Files and Object Files
#MPI_CPPFiles=$(shell find ./src/mpi -type f -name "*.cpp")
#MPI_ObjFiles=$(patsubst ./src/mpi/transport/%.cpp, ./bin/%.o, $(MPI_CPPFiles))
#
#NoMPI_CPPFiles=$(shell find ./src/nompi -type f -name "*NoMPI.cpp")
#NoMPI_ObjFiles=$(patsubst ./src/nompi/transport/%.cpp, ./bin/%.o, $(NoMPI_CPPFiles))
#
#Common_CPPFiles=$(wildcard ./src/*.cpp)
#Common_ObjFiles=$(patsubst ./src/%.cpp, ./bin/%.o, $(Common_CPPFiles))
#NoMPI_Common_ObjFiles=$(patsubst ./src/%.cpp, ./bin/%_nompi.o, $(Common_CPPFiles)) # use for nompi
#
#ObjFiles=$(MPI_ObjFiles) $(NoMPI_ObjFiles)


#Build all MPI and noMPI
#all: $(MPI_ObjFiles) $(NoMPI_ObjFiles) $(Common_ObjFiles) $(HFiles)
#	$(AR) rcs ./lib/libadios.a $(MPI_ObjFiles) $(NoMPI_ObjFiles) $(Common_ObjFiles)
#    
##NoMPI build    
#nompi: $(NoMPI_ObjFiles) $(NoMPI_Common_ObjFiles) $(NoMPI_HFiles) $(Local_HFiles)
#	$(AR) rcs ./lib/libadios_nompi.a $(NoMPI_ObjFiles) $(NoMPI_Common_ObjFiles)
#	
#./bin/%.o: ./src/nompi/transport/%.cpp $(NoMPI_HFiles) $(Local_HFiles)
#	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $<
#	
#./bin/%_nompi.o: ./src/%.cpp $(NoMPI_HFiles) $(Local_HFiles)
#	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $<
	
	
clean_nompi:
	rm ./bin/*_nompi.o ./lib/libadios_nompi.a

clean_mpi:
	rm ./bin/*.o ./lib/libadios.a

clean:
	rm ./bin/*.o ./lib/libadios.a ./lib/libadios_nompi.a
	
