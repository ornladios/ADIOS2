#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.6)
project(adios_fortran_test Fortran)

find_package(adios2 REQUIRED)

if(ADIOS2_HAVE_MPI)
  find_package(MPI REQUIRED)
  set(src main_mpi.f90)
  set(mpilib MPI::MPI_Fortran)
  set(mpiexec ${MPIEXEC_EXECUTABLE})
else()
  set(src main_nompi.f90)
  set(mpilib)
  set(mpiexec)
endif()

add_executable(adios_fortran_test ${src})
target_link_libraries(adios_fortran_test adios2::adios2_f ${mpilib})

enable_testing()
add_test(NAME test COMMAND ${mpiexec} $<TARGET_FILE:adios_fortran_test>)
