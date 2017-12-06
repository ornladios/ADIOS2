#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

find_package(MPI COMPONENTS C REQUIRED)

add_executable(hello_sstReader helloSstReader.cpp)
target_include_directories(hello_sstReader PRIVATE ${MPI_C_INCLUDE_PATH})
target_link_libraries(hello_sstReader ${MPI_C_LIBRARIES})
target_link_libraries(hello_sstReader adios2)
