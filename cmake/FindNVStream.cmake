#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindNVStream
# -----------
#
# Try to find the NVStream library
#
# This module defines the following variables:
#
#   NVStream_FOUND        - System has NVStream
#   NVStream_INCLUDE_DIRS - The NVStream include directory
#   NVStream_LIBRARIES    - Link these to use NVStream
#
# and the following imported targets:
#   NVStream::NVStream - The core NVStream library
#
# You can also set the following variable to help guide the search:
#   NVStream_ROOT - The install prefix for NVStream containing the
#                     include and lib folders
#                     Note: this can be set as a CMake variable or an
#                           environment variable.  If specified as a CMake
#                           variable, it will override any setting specified
#                           as an environment variable.

if(CMAKE_VERSION VERSION_LESS 3.12)
  if((NOT NVStream_ROOT) AND (NOT (ENV{NVStream_ROOT} STREQUAL "")))
    set(NVStream_ROOT "$ENV{NVStream_ROOT}")
  endif()
  if(NVStream_ROOT)
    set(NVStream_INCLUDE_OPTS HINTS ${NVStream_ROOT}/include NO_DEFAULT_PATHS)
    set(NVStream_LIBRARY_OPTS
      HINTS ${NVStream_ROOT}/lib ${NVStream_ROOT}/lib64
      NO_DEFAULT_PATHS
    )
  endif()
endif()

find_path(NVStream_INCLUDE_DIR nvs/store.h ${NVStream_INCLUDE_OPTS})
find_library(NVStream_LIBRARY libyuma.a ${NVStream_LIBRARY_OPTS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NVStream
  FOUND_VAR NVStream_FOUND
  REQUIRED_VARS NVStream_LIBRARY NVStream_INCLUDE_DIR
)

if(NVStream_FOUND)
  set(NVStream_INCLUDE_DIRS ${NVStream_INCLUDE_DIR})
  set(NVStream_LIBRARIES ${NVStream_LIBRARY})
  if(NOT TARGET NVStream::NVStream)
    add_library(NVStream::NVStream UNKNOWN IMPORTED)
    set_target_properties(NVStream::NVStream PROPERTIES
      IMPORTED_LOCATION             "${NVStream_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${NVStream_INCLUDE_DIR}"
    )
  endif()
endif()
