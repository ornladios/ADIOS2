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

add_library(dataman
  DataManBase.cpp DataManBase.h
  DataMan.cpp DataMan.h
  CacheMan.cpp CacheMan.h
)
target_include_directories(dataman
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(dataman
  PRIVATE adios2sys
  PUBLIC NLohmannJson
)
set_target_properties(dataman PROPERTIES
  VERSION ${ADIOS2_VERSION}
  SOVERSION ${ADIOS2_VERSION_MAJOR}
)
install(TARGETS dataman EXPORT adios2Exports
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dataman
)

# Add the dataman plugins as MODULE libraries instead of SHARED libraries.
# MODULE libraries are designed to be plugins, i.e. shared libs that nobody
# else links to.

set(dataman_modules)

add_library(dumpman MODULE DumpMan.h DumpMan.cpp)
target_link_libraries(dumpman PRIVATE dataman)
list(APPEND dataman_modules dumpman)

add_library(temporalman MODULE TemporalMan.h TemporalMan.cpp)
target_link_libraries(temporalman PRIVATE dataman)
list(APPEND dataman_modules temporalman)

if(ADIOS2_HAVE_ZeroMQ)
  add_library(zmqman MODULE
    StreamMan.h StreamMan.cpp
    ZmqMan.h ZmqMan.cpp
  )
  target_link_libraries(zmqman PRIVATE dataman ZeroMQ::ZMQ)
  list(APPEND dataman_modules zmqman)

  add_library(mdtmman MODULE
    StreamMan.h StreamMan.cpp
    MdtmMan.h MdtmMan.cpp
  )
  target_link_libraries(mdtmman PRIVATE dataman ZeroMQ::ZMQ)
  list(APPEND dataman_modules mdtmman)
endif()

if(ADIOS2_HAVE_ZFP)
  add_library(zfpman MODULE ZfpMan.h ZfpMan.cpp)
  target_link_libraries(zfpman PRIVATE dataman zfp::zfp)

  list(APPEND dataman_modules zfpman)
endif()

install(TARGETS ${dataman_modules}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
