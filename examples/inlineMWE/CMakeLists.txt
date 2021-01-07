#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(inlineMWE inlineMWE.cpp)
target_link_libraries(inlineMWE adios2::cxx11)
