#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_timeBPWriter timeBPWriter.cpp)
target_link_libraries(hello_timeBPWriter adios2)

add_executable(hello_timeBPWriter_nompi timeBPWriter_nompi.cpp)
target_link_libraries(hello_timeBPWriter_nompi adios2)
