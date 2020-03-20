#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

ADIOS_CXX_FLAGS=$(shell adios2-config --cxx-flags)
ADIOS_CXX_LD_FLAGS=$(shell adios2-config --cxx-libs)
ADIOS_CXX_ALL_FLAGS=$(shell adios2-config -x)

ifeq ($(ADIOS2_HAVE_MPI),1)
	main_cxx=main_mpi.cxx
	mpiexec=$(MPIEXEC)
else
	main_cxx=main_nompi.cxx
	mpiexec=
endif

all: test

test: adios_cxx11_test adios_cxx11_test_2
	./adios_cxx11_test
	./adios_cxx11_test_2

adios_cxx11_test: main.o
	$(CXX) $(LDFLAGS) $(CXXFLAGS) -o adios_cxx11_test main.o $(ADIOS_CXX_LD_FLAGS)

main.o: $(main_cxx)
	$(CXX) $(CXXFLAGS) $(ISYSROOT) -o main.o -c $(main_cxx) $(ADIOS_CXX_FLAGS)

adios_cxx11_test_2: $(main_cxx)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) $(ISYSROOT) -o adios_cxx11_test_2 $(main_cxx) $(ADIOS_CXX_ALL_FLAGS)
