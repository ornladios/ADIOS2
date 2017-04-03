#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_datamanReader helloDataManReader.cpp)
target_link_libraries(hello_datamanReader adios2)

add_executable(hello_datamanReader_nompi helloDataManReader_nompi.cpp)
target_link_libraries(hello_datamanReader_nompi adios2)
