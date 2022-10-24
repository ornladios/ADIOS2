#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindBLOSC22
# -----------
#
# Try to find the BLOSC2 library
#
# This module defines the following variables:
#
#   BLOSC2_FOUND        - System has BLOSC2
#   BLOSC2_INCLUDE_DIRS - The BLOSC2 include directory
#   BLOSC2_LIBRARIES    - Link these to use BLOSC2
#   BLOSC2_VERSION      - Version of the BLOSC2 library to support
#
# and the following imported targets:
#   Blosc2::Blosc2 - The core BLOSC2 library
#
# You can also set the following variable to help guide the search:
#   BLOSC2_ROOT - The install prefix for BLOSC2 containing the
#                     include and lib folders
#                     Note: this can be set as a CMake variable or an
#                           environment variable.  If specified as a CMake
#                           variable, it will override any setting specified
#                           as an environment variable.

if(NOT BLOSC2_FOUND)
  if((NOT BLOSC2_ROOT) AND (DEFINED ENV{BLOSC2_ROOT}))
    set(BLOSC2_ROOT "$ENV{BLOSC2_ROOT}")
  endif()
  if(BLOSC2_ROOT)
    set(BLOSC2_INCLUDE_OPTS HINTS ${BLOSC2_ROOT}/include NO_DEFAULT_PATHS)
    set(BLOSC2_LIBRARY_OPTS
      HINTS ${BLOSC2_ROOT}/lib ${BLOSC2_ROOT}/lib64
      NO_DEFAULT_PATHS
    )
  endif()
  if(WIN32) # uses a Unix-like library prefix on Windows
    set(BLOSC2_LIBRARY_OPTS
      libblosc2 ${BLOSC2_LIBRARY_OPTS}
    )
  endif()

  find_path(BLOSC2_INCLUDE_DIR blosc2.h ${BLOSC2_INCLUDE_OPTS})
  find_library(BLOSC2_LIBRARY NAMES blosc2 ${BLOSC2_LIBRARY_OPTS})
  if(BLOSC2_INCLUDE_DIR)
    file(STRINGS ${BLOSC2_INCLUDE_DIR}/blosc2.h _ver_string
      REGEX [=[BLOSC2_VERSION_STRING +"[^"]*"]=]
    )
    if(_ver_string MATCHES [=[BLOSC2_VERSION_STRING +"([0-9]+.[0-9]+.[0-9]+)]=])
      set(BLOSC2_VERSION ${CMAKE_MATCH_1})
    endif()
  endif()

  # Blosc2 depends on pthreads
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  if(WIN32)
    # try to use the system library
    find_package(Threads)
    if(NOT Threads_FOUND)
      message(STATUS "Blosc2: used the internal pthread library for win32 systems.")
      set(BLOSC2_LIBRARIES)
    else()
      set(BLOSC2_LIBRARIES Threads::Threads)
    endif()
  else()
    find_package(Threads REQUIRED)
    set(BLOSC2_LIBRARIES Threads::Threads)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Blosc2
    FOUND_VAR BLOSC2_FOUND
    VERSION_VAR BLOSC2_VERSION
    REQUIRED_VARS BLOSC2_LIBRARY BLOSC2_INCLUDE_DIR
  )
  if(BLOSC2_FOUND)
    set(BLOSC2_INCLUDE_DIRS ${BLOSC2_INCLUDE_DIR})
    set(BLOSC2_LIBRARIES ${BLOSC2_LIBRARY})
    if(BLOSC2_FOUND AND NOT TARGET Blosc2::Blosc2)
      add_library(Blosc2::Blosc2 UNKNOWN IMPORTED)
      set_target_properties(Blosc2::Blosc2 PROPERTIES
        IMPORTED_LOCATION             "${BLOSC2_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${BLOSC2_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES      "${BLOSC2_LIBRARIES}"
      )
    endif()
  endif()
endif()
