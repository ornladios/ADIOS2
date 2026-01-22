#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# This module is already included in new versions of CMake
if(CMAKE_VERSION VERSION_LESS 3.19)
  include(${CMAKE_CURRENT_LIST_DIR}/upstream/FindHDF5.cmake)
else()
  include(${CMAKE_ROOT}/Modules/FindHDF5.cmake)
endif()

if ( HDF5_PROVIDES_PARALLEL )   
    if(NOT DEFINED HDF5_IS_PARALLEL)  
       message (STATUS "ADIOS is double checking to use the HDF2.0 parallel flag")      
       set(HDF5_IS_PARALLEL ${HDF5_PROVIDES_PARALLEL})
       message (STATUS "HDF is set to parralel: ${HDF5_IS_PARALLEL}")
     endif()
endif()

