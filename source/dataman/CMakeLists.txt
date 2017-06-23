#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(NOT SHARED_LIBS_SUPPORTED)
  message(FATAL_ERROR "DataMan requires shared library support")
endif()
if(MSVC)
  message(FATAL_ERROR "DataMan is not currently compatible with MSVC")
endif()

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

if(ADIOS2_HAVE_ZeroMQ)
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

if(ADIOS2_HAVE_ZFP)
  find_package(ZFP REQUIRED)

  add_library(zfpman MODULE ZfpMan.h ZfpMan.cpp)
  target_link_libraries(zfpman PRIVATE dataman zfp::zfp)

  list(APPEND dataman_targets zfpman)
endif()

# Library versioning like this only makes sense for libdataman.  The others are plugins
# loaded via dlopen.
set_target_properties(dataman PROPERTIES
  VERSION ${ADIOS2_VERSION}
  SOVERSION ${ADIOS2_VERSION_MAJOR}
)

install(
  TARGETS ${dataman_targets} EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
