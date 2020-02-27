#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# This module is already included in new versions of CMake
if(CMAKE_VERSION VERSION_LESS 3.16.4)
  include(${CMAKE_CURRENT_LIST_DIR}/upstream/FindPython.cmake)
else()
  include(${CMAKE_ROOT}/Modules/FindPython.cmake)
endif()
