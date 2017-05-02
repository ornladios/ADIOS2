#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindZeroMQ
# -----------
#
# Try to find the ZeroMQ library
#
# This module defines the following variables:
#
#   ZeroMQ_FOUND        - System has ZMQ
#   ZeroMQ_INCLUDE_DIRS - The ZMQ include directory
#   ZeroMQ_LIBRARIES    - Link these to use ZMQ
#
# and the following imported targets:
#   ZeroMQ::ZMQ - The core ZMQ library
#
# You can also set the following variable to help guide the search:
#   ZeroMQ_ROOT_DIR - The install prefix for ZeroMQ containing the
#                     include and lib folders
#                     Note: this can be set as a CMake variable or an
#                           environment variable.  If specified as a CMake
#                           variable, it will override any setting specified
#                           as an environment variable.

if(NOT ZeroMQ_FOUND)
  if((NOT ZeroMQ_ROOT_DIR) AND (NOT (ENV{ZeroMQ_ROOT_DIR} STREQUAL "")))
    set(ZeroMQ_ROOT_DIR "$ENV{ZeroMQ_ROOT_DIR}")
  endif()
  if(ZeroMQ_ROOT_DIR)
    set(ZeroMQ_INCLUDE_OPTS HINTS ${ZeroMQ_ROOT_DIR}/include NO_DEFAULT_PATHS)
    set(ZeroMQ_LIBRARY_OPTS
      HINTS ${ZeroMQ_ROOT_DIR}/lib ${ZeroMQ_ROOT_DIR}/lib64
      NO_DEFAULT_PATHS
    )
  endif()

  find_path(ZeroMQ_INCLUDE_DIR zmq.h ${ZeroMQ_INCLUDE_OPTS})
  find_library(ZeroMQ_LIBRARY zmq ${ZeroMQ_LIBRARY_OPTS})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(ZeroMQ
    FOUND_VAR ZeroMQ_FOUND
    REQUIRED_VARS ZeroMQ_LIBRARY ZeroMQ_INCLUDE_DIR
  )
  if(ZeroMQ_FOUND)
    set(ZeroMQ_INCLUDE_DIRS ${ZeroMQ_INCLUDE_DIR})
    set(ZeroMQ_LIBRARIES ${ZeroMQ_LIBRARY})
    if(ZeroMQ_FOUND AND NOT TARGET ZeroMQ::ZMQ)
      add_library(ZeroMQ::ZMQ UNKNOWN IMPORTED)
      set_target_properties(ZeroMQ::ZMQ PROPERTIES
        IMPORTED_LOCATION             "${ZeroMQ_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ZeroMQ_INCLUDE_DIR}"
      )
    endif()
  endif()
endif()
