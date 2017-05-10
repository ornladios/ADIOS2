#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(dataman_targets)

add_library(dataman
  DataManBase.cpp DataManBase.h
  DataMan.cpp DataMan.h
  CacheMan.cpp CacheMan.h
)
target_include_directories(dataman PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dataman
  PRIVATE adios2sys
  PUBLIC NLohmannJson
)
list(APPEND dataman_targets dataman)

# Add the dataman plugins as MODULE libraries instead of SHARED libraries.
# MODULE libraries are designed to be plugins, i.e. shared libs that nobody
# else links to.

add_library(dumpman MODULE DumpMan.h DumpMan.cpp)
target_link_libraries(dumpman PRIVATE dataman)
list(APPEND dataman_targets dumpman)

add_library(temporalman MODULE TemporalMan.h TemporalMan.cpp)
target_link_libraries(temporalman PRIVATE dataman)
list(APPEND dataman_targets temporalman)

option(ADIOS_USE_DataMan_ZeroMQ "Enable ZeroMQ for DataMan" OFF)
if(ADIOS_USE_DataMan_ZeroMQ)
  find_package(ZeroMQ REQUIRED)

  add_library(zmqman MODULE
    StreamMan.h StreamMan.cpp
    ZmqMan.h ZmqMan.cpp
  )
  target_link_libraries(zmqman PRIVATE dataman ZeroMQ::ZMQ)
  list(APPEND dataman_targets zmqman)

  add_library(mdtmman MODULE
    StreamMan.h StreamMan.cpp
    MdtmMan.h MdtmMan.cpp
  )
  target_link_libraries(mdtmman PRIVATE dataman ZeroMQ::ZMQ)
  list(APPEND dataman_targets mdtmman)
endif()


set(ADIOS_USE_DataMan_ZFP ${ADIOS_USE_ZFP} CACHE INTERNAL "Enable ZFP for DataMan" FORCE)
if(ADIOS_USE_DataMan_ZFP)
  find_package(ZFP REQUIRED)

  add_library(zfpman MODULE ZfpMan.h ZfpMan.cpp)
  target_link_libraries(zfpman PRIVATE dataman zfp::zfp)

  list(APPEND dataman_targets zfpman)
endif()

install(
  TARGETS ${dataman_targets} EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
