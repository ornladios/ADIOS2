#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

ADIOS_CXX_FLAGS=$(shell adios2-config --cxx-flags)
ADIOS_CXX_LD_FLAGS=$(shell adios2-config --cxx-libs)
ADIOS_CXX_ALL_FLAGS=$(shell adios2-config -x)

all: test

test: adios_cxx11_test adios_cxx11_test_2
	./adios_cxx11_test
	./adios_cxx11_test_2

adios_cxx11_test: main.o
	$(CXX) -o adios_cxx11_test main.o $(ADIOS_CXX_LD_FLAGS)

main.o: main.cxx
	$(CXX) $(ISYSROOT) -o main.o -c main.cxx $(ADIOS_CXX_FLAGS)

adios_cxx11_test_2: main.cxx
	$(CXX) $(ISYSROOT) -o adios_cxx11_test_2 main.cxx $(ADIOS_CXX_ALL_FLAGS)
