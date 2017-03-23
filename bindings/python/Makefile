#Makefile
# Created on: Mar 13, 2017
#     Author: wfg

MPICC:=mpic++

CPPFiles:=$(shell find ./src -type f -name "*.cpp")
OBJS:=$(patsubst %.cpp, ./bin/%.o, $(notdir $(CPPFiles)) )

# location of the Python header files
PYTHON_VERSION:= 2.7
PYTHON_INC:= /usr/include/python$(PYTHON_VERSION)
PYTHON_LIBLOC:= /usr/lib/python$(PYTHON_VERSION)/config-x86_64-linux-gnu/
 

ifeq ($(HAVE_PYBIND11),yes)  # USE PYBIND11 (headers only)
    BIND_FLAG:= -DHAVE_PYBIND11
    BIND_INC:= /home/wfg/workspace/pybind11/include
    BIND_LIBS:=
    OBJS:=$(patsubst ./bin/glueBoostPython.o, ,$(OBJS) ) #remove BoostPython
else                             # USE BOOST PYTHON (default)
	BIND_FLAG:= -DHAVE_BOOSTPYTHON
	BIND_INC:= /opt/boost_1_63_0/
	BIND_LIBS:= -L/opt/boost_1_63_0/stage/lib/ -lboost_python -lboost_numpy
	OBJS:=$(patsubst ./bin/gluePyBind11.o, ,$(OBJS) ) #remove PyBind11
endif


# location of ADIOS include and lib
ADIOS_INC:= /home/wfg/workspace/ADIOSPP/include
ADIOS_LIB:= /home/wfg/workspace/ADIOSPP/lib
ADIOSPy_INC:=./include


CFLAGS:=-c -Wall -fPIC -O0 -g -std=c++11 -Wno-deprecated-declarations $(BIND_FLAG)
INC:= -I$(PYTHON_INC) -I$(BIND_INC) -I$(ADIOS_INC) -I$(ADIOSPy_INC)
LIBS:= -L$(ADIOS_LIB) -ladios $(BIND_LIBS) -L$(PYTHON_LIBLOC) -lpython$(PYTHON_VERSION)

 
# compile mesh classes
TARGET = ADIOSPy

all: $(TARGET).so
 
$(TARGET).so: $(OBJS)
	$(MPICC) -shared -o $(TARGET).so -Wl,--export-dynamic $(OBJS) $(LIBS) 
 
./bin/%.o: ./src/%.cpp
	@( mkdir -p ./bin );
	$(MPICC) $(INC) $(CFLAGS) -o $@ $<
	
clean:
	rm ./bin/*.o *.so