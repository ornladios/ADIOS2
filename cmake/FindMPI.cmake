#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# This module is already included in new versions of CMake
if(CMAKE_VERSION VERSION_LESS 3.15.1)
  include(${CMAKE_CURRENT_LIST_DIR}/upstream/FindMPI.cmake)
else()
  include(${CMAKE_ROOT}/Modules/FindMPI.cmake)
endif()
