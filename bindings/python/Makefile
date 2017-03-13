#Makefile
# Created on: Mar 13, 2017
#     Author: wfg

# location of the Python header files
PYTHON_VERSION:= 2.7
PYTHON_INC:= /usr/include/python$(PYTHON_VERSION)
PYTHON_LIBLOC:= /usr/lib/python2.7/config-x86_64-linux-gnu/
 
# location of the Boost Python include files and library 
BOOST_INC:= /usr/include/boost
BOOST_LIBLOC:= /lib/x86_64-linux-gnu/

# location of ADIOS include and lib
ADIOS_INC:= /home/wfg/workspace/ADIOSPP/include
ADIOS_LIB:= /home/wfg/workspace/ADIOSPP/lib
ADIOSPy_INC:=./include

CFLAGS:=-c -Wall -fPIC -O0 -g -std=c++11

CPPFiles:=$(shell find ./src -type f -name "*.cpp")
OBJS:=$(patsubst %.cpp, ./bin/%.o, $(notdir $(CPPFiles)) )

INC:= -I$(PYTHON_INC) -I$(BOOST_INC) -I$(ADIOS_INC) -I$(ADIOSPy_INC)
LIBS:= -L$(ADIOS_LIB) -ladios -L$(BOOST_LIBLOC) -lboost_python-py27 -L$(PYTHON_LIBLOC) -lpython$(PYTHON_VERSION)

 
# compile mesh classes
TARGET = ADIOSPy

all: $(TARGET).so
 
$(TARGET).so: $(OBJS)
	mpic++ -shared -o $(TARGET).so -Wl,--export-dynamic $(OBJS) $(LIBS) 
 
./bin/%.o: ./src/%.cpp
	@( mkdir -p ./bin );
	mpic++ $(INC) $(CFLAGS) -o $@ $<
	
clean:
	rm ./bin/*.o *.so