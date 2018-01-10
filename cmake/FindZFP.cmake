#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindZFP
# -----------
#
# Try to find the zfp library
#
# This module defines the following variables:
#
#   ZFP_FOUND        - System has zfp
#   ZFP_INCLUDE_DIRS - The zfp include directory
#   ZFP_LIBRARIES    - Link these to use zfp
#
# and the following imported targets:
#   zfp::zfp - The zfp compression library target
#
# You can also set the following variable to help guide the search:
#   ZFP_ROOT - The install prefix for zfp containing the
#              include and lib folders
#              Note: this can be set as a CMake variable or an
#                    environment variable.  If specified as a CMake
#                    variable, it will override any setting specified
#                    as an environment variable.

if(NOT ZFP_FOUND)
  if((NOT ZFP_ROOT) AND (NOT (ENV{ZFP_ROOT} STREQUAL "")))
    set(ZFP_ROOT "$ENV{ZFP_ROOT}")
  endif()
  if(ZFP_ROOT)
    set(ZFP_INCLUDE_OPTS HINTS ${ZFP_ROOT}/include NO_DEFAULT_PATHS)
    set(ZFP_LIBRARY_OPTS
      HINTS ${ZFP_ROOT}/lib ${ZFP_ROOT}/lib64
      NO_DEFAULT_PATHS
    )
  endif()

  find_path(ZFP_INCLUDE_DIR zfp.h ${ZFP_INCLUDE_OPTS})
  find_library(ZFP_LIBRARY zfp ${ZFP_LIBRARY_OPTS})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(ZFP
    FOUND_VAR ZFP_FOUND
    REQUIRED_VARS ZFP_LIBRARY ZFP_INCLUDE_DIR
  )
  if(ZFP_FOUND)
    set(ZFP_INCLUDE_DIRS ${ZFP_INCLUDE_DIR})
    set(ZFP_LIBRARIES ${ZFP_LIBRARY})
    if(ZFP_FOUND AND NOT TARGET zfp::zfp)
      add_library(zfp::zfp UNKNOWN IMPORTED)
      set_target_properties(zfp::zfp PROPERTIES
        IMPORTED_LOCATION             "${ZFP_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ZFP_INCLUDE_DIR}"
      )
    endif()
  endif()
endif()
