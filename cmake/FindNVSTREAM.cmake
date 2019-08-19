#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindNVSTREAM
# -----------
#
# Try to find the NVSTREAM library
#
# This module defines the following variables:
#
#   NVSTREAM_FOUND        - System has NVSTREAM
#   NVSTREAM_INCLUDE_DIRS - The NVSTREAM include directory
#   NVSTREAM_LIBRARIES    - Link these to use NVSTREAM
#   NVSTREAM_VERSION      - Version of the NVSTREAM library to support
#
# and the following imported targets:
#   NVStream::NVStream - The core NVSTREAM library
#
# You can also set the following variable to help guide the search:
#   NVSTREAM_ROOT - The install prefix for NVSTREAM containing the
#                     include and lib folders
#                     Note: this can be set as a CMake variable or an
#                           environment variable.  If specified as a CMake
#                           variable, it will override any setting specified
#                           as an environment variable.

if(NOT NVSTREAM_FOUND)
  if((NOT NVSTREAM_ROOT) AND (NOT (ENV{NVSTREAM_ROOT} STREQUAL "")))
    set(NVSTREAM_ROOT "$ENV{NVSTREAM_ROOT}")
  endif()
  if(NVSTREAM_ROOT)
    set(NVSTREAM_INCLUDE_OPTS HINTS ${NVSTREAM_ROOT}/include NO_DEFAULT_PATHS)
    set(NVSTREAM_LIBRARY_OPTS
      HINTS ${NVSTREAM_ROOT}/lib ${NVSTREAM_ROOT}/lib64
      NO_DEFAULT_PATHS
    )
  endif()

  find_path(NVSTREAM_INCLUDE_DIR nvs/store.h ${NVSTREAM_INCLUDE_OPTS})
  find_library(NVSTREAM_LIBRARY libyuma.a ${NVSTREAM_LIBRARY_OPTS})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(NVStream
    FOUND_VAR NVSTREAM_FOUND
    VERSION_VAR NVSTREAM_VERSION
    REQUIRED_VARS NVSTREAM_LIBRARY NVSTREAM_INCLUDE_DIR
  )
    message(STATUS "NVSTREAM_FOUND is ${NVSTREAM_FOUND}, LIB is ${NVSTREAM_LIBRARY}")
  if(NVSTREAM_FOUND)
    set(NVSTREAM_INCLUDE_DIRS ${NVSTREAM_INCLUDE_DIR})
    set(NVSTREAM_LIBRARIES ${NVSTREAM_LIBRARY})
    if(NVSTREAM_FOUND AND NOT TARGET NVStream::NVStream)
        message(STATUS "ADDING LIBRARY NVSTREAM_FOUND is ${NVSTREAM_FOUND}, LIB is ${NVSTREAM_LIBRARY}")
      add_library(NVStream::NVStream UNKNOWN IMPORTED)
      set_target_properties(NVStream::NVStream PROPERTIES
        IMPORTED_LOCATION             "${NVSTREAM_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${NVSTREAM_INCLUDE_DIR}"
      )
    endif()
  endif()
endif()
