#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_skeletonWriter helloSkeletonWriter.cpp HelloSkeletonArgs.cpp)
target_link_libraries(hello_skeletonWriter adios2::cxx11)

add_executable(hello_skeletonReader helloSkeletonReader.cpp HelloSkeletonArgs.cpp)
target_link_libraries(hello_skeletonReader adios2::cxx11)

if(ADIOS2_HAVE_MPI)
  add_executable(hello_skeletonWriter_mpi helloSkeletonWriter.cpp HelloSkeletonArgs.cpp)
  target_link_libraries(hello_skeletonWriter_mpi adios2::cxx11_mpi MPI::MPI_C)

  add_executable(hello_skeletonReader_mpi helloSkeletonReader.cpp HelloSkeletonArgs.cpp)
  target_link_libraries(hello_skeletonReader_mpi adios2::cxx11_mpi MPI::MPI_C)
endif()
