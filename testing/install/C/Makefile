#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

ADIOS_C_FLAGS=$(shell adios2-config --c-flags)
ADIOS_C_LD_FLAGS=$(shell adios2-config --c-libs)
ADIOS_C_ALL_FLAGS=$(shell adios2-config -c)

ifeq ($(ADIOS2_HAVE_MPI),1)
	main_c=main_mpi.c
	mpiexec=$(MPIEXEC)
else
	main_c=main_nompi.c
	mpiexec=
endif

all: test

test: adios_c_test adios_c_test_2
	$(mpiexec) ./adios_c_test
	$(mpiexec) ./adios_c_test_2

adios_c_test: main.o
	$(CXX) -o adios_c_test main.o $(ADIOS_C_LD_FLAGS)

main.o: $(main_c)
	$(CC) $(ISYSROOT) -o main.o -c $(main_c) $(ADIOS_C_FLAGS)

adios_c_test_2: $(main_c)
	$(CXX) $(ISYSROOT) -o adios_c_test_2 $(main_c) $(ADIOS_C_ALL_FLAGS)
