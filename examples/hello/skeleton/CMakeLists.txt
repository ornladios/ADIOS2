#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_skeletonWriter helloSkeletonWriter.cpp HelloSkeletonArgs.cpp)
add_executable(hello_skeletonReader helloSkeletonReader.cpp HelloSkeletonArgs.cpp)

if(ADIOS2_HAVE_MPI)
  target_include_directories(hello_skeletonWriter PRIVATE ${MPI_C_INCLUDE_PATH})
  target_include_directories(hello_skeletonReader PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_skeletonWriter MPI::MPI_C)
  target_link_libraries(hello_skeletonReader MPI::MPI_C)
endif()
 
target_link_libraries(hello_skeletonWriter adios2)
target_link_libraries(hello_skeletonReader adios2)


