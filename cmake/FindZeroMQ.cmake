# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0


if(NOT ZeroMQ_FOUND)
  if((NOT ZeroMQ_ROOT) AND (NOT (ENV{ZeroMQ_ROOT} STREQUAL "")))
    set(ZeroMQ_ROOT "$ENV{ZeroMQ_ROOT}")
  endif()
  if(ZeroMQ_ROOT)
    set(ZeroMQ_INCLUDE_OPTS HINTS ${ZeroMQ_ROOT}/include NO_DEFAULT_PATHS)
    set(ZeroMQ_LIBRARY_OPTS
      HINTS ${ZeroMQ_ROOT}/lib ${ZeroMQ_ROOT}/lib64
      NO_DEFAULT_PATHS
    )
  endif()

  find_path(ZeroMQ_INCLUDE_DIR zmq.h ${ZeroMQ_INCLUDE_OPTS})
  find_library(ZeroMQ_LIBRARY zmq ${ZeroMQ_LIBRARY_OPTS})
  if(ZeroMQ_INCLUDE_DIR)
    file(STRINGS ${ZeroMQ_INCLUDE_DIR}/zmq.h _ver_strings
      REGEX "ZMQ_VERSION_[^ ]* [0-9]+"
    )
    foreach(v IN LISTS _ver_strings)
      string(REGEX MATCH "ZMQ_VERSION_([^ ]+) ([0-9]+)" v "${v}")
      set(ZeroMQ_VERSION_${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
    endforeach()
    set(ZeroMQ_VERSION
      ${ZeroMQ_VERSION_MAJOR}.${ZeroMQ_VERSION_MINOR}.${ZeroMQ_VERSION_PATCH}
    )
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(ZeroMQ
    FOUND_VAR ZeroMQ_FOUND
    VERSION_VAR ZeroMQ_VERSION
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
