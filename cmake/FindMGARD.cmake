#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindMGARD
# -----------
#
# Try to find the MGARD library
#
# This module defines the following variables:
#
#   MGARD_FOUND        - System has MGARD
#   MGARD_INCLUDE_DIRS - The MGARD include directory
#   MGARD_LIBRARIES    - Link these to use MGARD
#
# and the following imported targets:
#   MGARD::MGARD - The MGARD compression library target
#
# You can also set the following variable to help guide the search:
#   MGARD_ROOT - The install prefix for MGARD containing the
#              include and lib folders
#              Note: this can be set as a CMake variable or an
#                    environment variable.  If specified as a CMake
#                    variable, it will override any setting specified
#                    as an environment variable.

if(NOT MGARD_FOUND)
  if((NOT MGARD_ROOT) AND (NOT (ENV{MGARD_ROOT} STREQUAL "")))
    set(MGARD_ROOT "$ENV{MGARD_ROOT}")
  endif()
  if(MGARD_ROOT)
    set(MGARD_INCLUDE_OPTS HINTS ${MGARD_ROOT}/include NO_DEFAULT_PATHS)
    set(MGARD_LIBRARY_OPTS
      HINTS ${MGARD_ROOT}/lib ${MGARD_ROOT}/lib64
      NO_DEFAULT_PATHS
    )
  endif()

  find_path(MGARD_INCLUDE_DIR mgard_api.h ${MGARD_INCLUDE_OPTS})
  find_library(MGARD_LIBRARY NAMES mgard ${MGARD_LIBRARY_OPTS})
  find_library(ZLIB_LIBRARY NAMES zlib z ${ZLIB_LIBRARY_OPTS})
  find_library(ZSTD_LIBRARY NAMES zstd ${ZSTD_LIBRARY_OPTS})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(MGARD
    FOUND_VAR MGARD_FOUND
    REQUIRED_VARS MGARD_LIBRARY ZLIB_LIBRARY ZSTD_LIBRARY MGARD_INCLUDE_DIR
  )
  if(MGARD_FOUND)
    set(MGARD_INCLUDE_DIRS ${MGARD_INCLUDE_DIR})
    set(MGARD_LIBRARIES ${MGARD_LIBRARY} ${ZLIB_LIBRARY} ${ZSTD_LIBRARY})
    if(MGARD_FOUND AND NOT TARGET MGARD::MGARD)
      add_library(MGARD::MGARD UNKNOWN IMPORTED)
      set_target_properties(MGARD::MGARD PROPERTIES
        IMPORTED_LOCATION             "${MGARD_LIBRARY}"
        INTERFACE_LINK_LIBRARIES      "${MGARD_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${MGARD_INCLUDE_DIR}"
      )
    endif()
  endif()
endif()
