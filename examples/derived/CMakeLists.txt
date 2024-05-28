#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(adios2_read_write_derived_serial read_write_derived.cpp)
target_link_libraries(adios2_read_write_derived_serial adios2::cxx11 adios2_core)
add_executable(adios2_write_derived_serial write_derived.cpp)
target_link_libraries(adios2_write_derived_serial adios2::cxx11 adios2_core)
