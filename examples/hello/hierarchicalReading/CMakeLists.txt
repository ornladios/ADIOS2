#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hierarchical_reading hierarchicalReading.cpp)
target_link_libraries(hierarchical_reading adios2::cxx11)
