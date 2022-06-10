#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(manyvars)
add_subdirectory(query)
add_subdirectory(metadata)

# just for executing manually for performance studies
add_executable(PerfVector PerfVector.cpp)
target_link_libraries(PerfVector adios2_core)