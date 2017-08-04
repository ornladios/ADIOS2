

CXX=mpic++
CXXFLAGS=-O0 -g -Wall -std=c++11 -fPIC
BASE=adios2_c

ADIOS2_DIR=/home/wgodoy/tmp/adios2

all: $(BASE).h $(BASE).o $(BASE)_enums.h
	$(CXX) -shared $(BASE).o  -o libadios2_c.so -I./ -I$(ADIOS2_DIR)/include -L$(ADIOS2_DIR)/lib -ladios2 -ldataman
    

$(BASE).o: $(BASE).cpp
	$(CXX) $(CXXFLAGS) $(BASE).cpp -c -I./ -I$(ADIOS2_DIR)/include
	
clean:
	rm libadios2_c.so $(BASE).o