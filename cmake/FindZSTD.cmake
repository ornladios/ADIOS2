#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindZSTD
# -----------
#
# Try to find the ZSTD library
#
# This module defines the following variables:
#
#   ZSTD_FOUND        - System has ZSTD
#   ZSTD_INCLUDE_DIRS - The ZSTD include directory
#   ZSTD_LIBRARIES    - Link these to use ZSTD
#
# and the following imported targets:
#   ZSTD::ZSTD - The ZSTD compression library target
#
# You can also set the following variable to help guide the search:
#   ZSTD_ROOT - The install prefix for ZSTD containing the
#              include and lib folders
#              Note: this can be set as a CMake variable or an
#                    environment variable.  If specified as a CMake
#                    variable, it will override any ZSTD_ROOT set in the
#                    environment.

if(NOT ZSTD_FOUND)
  if(NOT ZSTD_ROOT)
    if(NOT "$ENV{ZSTD_ROOT}" STREQUAL "")
      set(ZSTD_ROOT "$ENV{ZSTD_ROOT}")
    endif()
  endif()

  # Try to find the header
  find_path(ZSTD_INCLUDE_DIR zstd.h
    HINTS ${ZSTD_ROOT}
    PATH_SUFFIXES include
  )

  # Try to find the library
  find_library(ZSTD_LIBRARY zstd
    HINTS ${ZSTD_ROOT}
    PATH_SUFFIXES lib lib64
  )

  # Handle the QUIETLY and REQUIRED arguments and set ZSTD_FOUND to TRUE if all listed variables are TRUE.
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(ZSTD
    FOUND_VAR ZSTD_FOUND
    REQUIRED_VARS ZSTD_LIBRARY ZSTD_INCLUDE_DIR
  )

  if(ZSTD_FOUND)
    set(ZSTD_INCLUDE_DIRS ${ZSTD_INCLUDE_DIR})
    set(ZSTD_LIBRARIES ${ZSTD_LIBRARY})
    if(NOT TARGET ZSTD::ZSTD)
      add_library(ZSTD::ZSTD UNKNOWN IMPORTED)
      set_target_properties(ZSTD::ZSTD PROPERTIES
        IMPORTED_LOCATION "${ZSTD_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_INCLUDE_DIR}"
      )
    endif()
  endif()

  mark_as_advanced(ZSTD_INCLUDE_DIR ZSTD_LIBRARY)
endif()