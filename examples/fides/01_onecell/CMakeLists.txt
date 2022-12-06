#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(fides_onecell fides_onecell.cpp)
target_link_libraries(fides_onecell adios2::cxx11)
