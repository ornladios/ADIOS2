#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_datamanWriter helloDataManWriter.cpp)
target_link_libraries(hello_datamanWriter adios2)

add_executable(hello_datamanWriter_nompi helloDataManWriter_nompi.cpp)
target_link_libraries(hello_datamanWriter_nompi adios2)
