#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.6)
project(adios_cxx11_test CXX)

find_package(adios2 REQUIRED)

if(ADIOS2_HAVE_MPI)
  find_package(MPI REQUIRED)
  set(mpilib MPI::MPI_CXX)
  set(mpiexec ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS})
else()
  set(mpilib)
  set(mpiexec)
endif()

add_executable(adios_cxx11_test main.cxx)
target_link_libraries(adios_cxx11_test adios2::adios2 ${mpilib})

enable_testing()
add_test(NAME test COMMAND ${mpiexec} $<TARGET_FILE:adios_cxx11_test>)
