#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_bpWriter helloBPWriter.cpp)
target_link_libraries(hello_bpWriter adios2)

add_executable(hello_bpWriter_nompi helloBPWriter_nompi.cpp)
target_link_libraries(hello_bpWriter_nompi adios2)
