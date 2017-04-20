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

#option(ADIOS_USE_DataMan_ZeroMQ "Enable ZeroMQ for DataMan" OFF)
#if(ADIOS_USE_DataMan_ZeroMQ)
#  find_package(ZeroMQ REQUIRED)
#
#  add_library(zmqman MODULE ZmqMan.h ZmqMan.cpp)
#  target_link_libraries(zmqman PRIVATE dataman ZeroMQ::ZMQ)
#
#  list(APPEND dataman_targets zmqman)
#endif()

install(
  TARGETS ${dataman_targets} EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
