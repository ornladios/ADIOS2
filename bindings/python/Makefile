#Makefile
# Created on: Jun 8, 2017
#     Author: William F Godoy


MPICC:=mpic++

CPPFiles:=$(shell find ./source -type f -name "*.cpp")
OBJS:=$(patsubst %.cpp, ./bin/%.o, $(notdir $(CPPFiles)) )

# Python
PYTHON_VERSION:= 2.7
PYTHON_INC:= /usr/include/python$(PYTHON_VERSION)
PYTHON_LIBLOC:= /usr/lib/python$(PYTHON_VERSION)/config-x86_64-linux-gnu/
 

# ADIOS2
ADIOS2_INC:= /usr/local/include
ADIOS2_LIB:= /usr/local/lib

# PyBind11
PyBind11_INC:= /home/wfg/workspace/ADIOS2/thirdparty/pybind11/include


CFLAGS:=-c -Wall -fPIC -O0 -g -std=c++11 -Wno-deprecated-declarations
INC:= -I$(PYTHON_INC) -I$(ADIOS2_INC) -I$(PyBind11_INC)
LIBS:= -L$(ADIOS2_LIB) -ladios2 -L$(PYTHON_LIBLOC) -lpython$(PYTHON_VERSION)

 
# compile mesh classes
TARGET = adios2py

all: $(TARGET).so
 
$(TARGET).so: $(OBJS)
	$(MPICC) -shared -o $(TARGET).so -Wl,--export-dynamic $(OBJS) $(LIBS) 
 
./bin/%.o: ./source/%.cpp
	@( mkdir -p ./bin );
	$(MPICC) $(INC) $(CFLAGS) -o $@ $<
	
clean:
	rm ./bin/*.o *.so