#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(dataman_targets)

add_library(dataman
  DataManBase.h DataManBase.cpp
  DataMan.h DataMan.cpp
)
target_include_directories(dataman PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dataman PRIVATE adios2sys)
list(APPEND dataman_targets dataman)

# Add the dataman plugins as MODULE libraries instead of SHARED libraries.
# MODULE libraries are designed to be plugins, i.e. shared libs that nobody
# else links to.

add_library(cacheman MODULE CacheMan.h CacheMan.cpp)
target_link_libraries(cacheman PRIVATE dataman)
list(APPEND dataman_targets cacheman)

option(ADIOS_USE_DataMan_ZeroMQ "Enable ZeroMQ for DataMan" OFF)
if(ADIOS_USE_DataMan_ZeroMQ)
  find_package(ZeroMQ REQUIRED)
  message(STATUS "DataMan ZeroMQ support not yet implemented")

#  add_library(zmqman MODULE ZmqMan.h ZmqMan.cpp)
#  target_link_libraries(zmqman PRIVATE dataman ZeroMQ::ZMQ)
#
#  list(APPEND dataman_targets zmqman)
endif()

# Make this a standalone option if you want to seperately enable/disable
# DataMan ZFP support from ADIOS ZFP support.
set(ADIOS_USE_DataMan_ZFP ${ADIOS_USE_ZFP})

if(ADIOS_USE_DataMan_ZFP)
  find_package(ZFP REQUIRED)
  message(STATUS "DataMan ZFP support not yet implemented")

#  add_library(zfpman MODULE ZfpMan.h ZfpMan.cpp)
#  target_link_libraries(zfpman PRIVATE dataman zfp::zfp)
#
#  list(APPEND dataman_targets zfpman)
endif()

install(
  TARGETS ${dataman_targets} EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
