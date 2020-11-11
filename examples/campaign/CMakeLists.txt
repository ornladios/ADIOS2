#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(bpCamWriteRead bpWriteRead.cpp)
target_link_libraries(bpCamWriteRead adios2::cxx11)

if(ADIOS2_HAVE_MPI)
    add_executable(bpCamWriteRead_mpi bpWriteRead.cpp)
    target_link_libraries(bpCamWriteRead_mpi adios2::cxx11_mpi MPI::MPI_C)
endif()
