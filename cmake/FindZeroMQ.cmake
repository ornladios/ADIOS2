# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Locate libzmq and provide the ZeroMQ::ZeroMQ imported target.  A direct
# library search is tried first (CMAKE_PREFIX_PATH-aware, no pkg-config tool
# needed); pkg-config is the fallback for decorated library names like
# vcpkg's libzmq-mt-4_3_5 that a plain search misses.

if(NOT TARGET ZeroMQ::ZeroMQ)
  find_path(ZeroMQ_INCLUDE_DIR NAMES zmq.h)
  find_library(ZeroMQ_LIBRARY NAMES zmq)
  if(ZeroMQ_INCLUDE_DIR AND ZeroMQ_LIBRARY)
    add_library(ZeroMQ::ZeroMQ UNKNOWN IMPORTED)
    set_target_properties(ZeroMQ::ZeroMQ PROPERTIES
      IMPORTED_LOCATION "${ZeroMQ_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${ZeroMQ_INCLUDE_DIR}")
  else()
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
      pkg_check_modules(PC_ZeroMQ QUIET IMPORTED_TARGET libzmq)
      if(TARGET PkgConfig::PC_ZeroMQ)
        add_library(ZeroMQ::ZeroMQ INTERFACE IMPORTED)
        set_property(TARGET ZeroMQ::ZeroMQ
          PROPERTY INTERFACE_LINK_LIBRARIES PkgConfig::PC_ZeroMQ)
      endif()
    endif()
  endif()
endif()

if(TARGET ZeroMQ::ZeroMQ)
  set(ZeroMQ_TARGET ZeroMQ::ZeroMQ)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZeroMQ REQUIRED_VARS ZeroMQ_TARGET)

mark_as_advanced(ZeroMQ_INCLUDE_DIR ZeroMQ_LIBRARY)
