#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

ADIOS_F_FLAGS=$(shell adios2-config --fortran-flags)
ADIOS_F_LD_FLAGS=$(shell adios2-config --fortran-libs)
ADIOS_F_ALL_FLAGS=$(shell adios2-config -f)

ifeq ($(ADIOS2_HAVE_MPI),1)
	main_f90=main_mpi.f90
	mpiexec=$(MPIEXEC)
else
	main_f90=main_nompi.f90
	mpiexec=
endif

all: test

test: adios_fortran_test adios_fortran_test_2
	$(mpiexec) ./adios_fortran_test
	$(mpiexec) ./adios_fortran_test_2

adios_fortran_test: main.o
	$(FC) -o adios_fortran_test main.o $(ADIOS_F_LD_FLAGS)

main.o: $(main_f90)
	$(FC) -o main.o -c $(main_f90) $(ADIOS_F_FLAGS)

adios_fortran_test_2: $(main_f90)
	$(FC) -o adios_fortran_test_2 $(main_f90) $(ADIOS_F_ALL_FLAGS)
