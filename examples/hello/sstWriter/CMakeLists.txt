#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

find_package(MPI COMPONENTS C REQUIRED)
  
add_executable(hello_sstWriter helloSstWriter.cpp)
target_include_directories(hello_sstWriter PRIVATE ${MPI_C_INCLUDE_PATH})
target_link_libraries(hello_sstWriter ${MPI_C_LIBRARIES})
target_link_libraries(hello_sstWriter adios2)
