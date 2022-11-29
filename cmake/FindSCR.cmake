#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindSCR
# -----------
#
# Try to find the SCR library
#
# This module defines the following variables:
#
#   SCR_FOUND        - System has SCR
#   SCR_INCLUDE_DIRS - The SCR include directory
#   SCR_LIBRARIES    - Link these to use SCR
#
# and the following imported targets:
#   SCR::SCR - The SCR library target
#
# You can also set the following variable to help guide the search:
#   SCR_ROOT - The install prefix for SCR containing the
#              include and lib folders
#              Note: this can be set as a CMake variable or an
#                    environment variable.  If specified as a CMake
#                    variable, it will override any setting specified
#                    as an environment variable.

if(NOT SCR_FOUND)
  if((NOT SCR_ROOT) AND (NOT (ENV{SCR_ROOT} STREQUAL "")))
    set(SCR_ROOT "$ENV{SCR_ROOT}")
  endif()
  if(SCR_ROOT)
    set(SCR_INCLUDE_OPTS HINTS ${SCR_ROOT}/include NO_DEFAULT_PATHS)
    set(SCR_LIBRARY_OPTS
      HINTS ${SCR_ROOT}/lib ${SCR_ROOT}/lib64
      NO_DEFAULT_PATHS
    )
  endif()

  find_path(SCR_INCLUDE_DIR scr.h ${SCR_INCLUDE_OPTS})
  find_library(SCR_LIBRARY NAMES scr ${SCR_LIBRARY_OPTS})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(SCR
    FOUND_VAR SCR_FOUND
    REQUIRED_VARS SCR_LIBRARY SCR_INCLUDE_DIR
  )
  if(SCR_FOUND)
    set(SCR_INCLUDE_DIRS ${SCR_INCLUDE_DIR})
    set(SCR_LIBRARIES ${SCR_LIBRARY})
    if(SCR_FOUND AND NOT TARGET SCR::SCR)
      add_library(SCR::SCR UNKNOWN IMPORTED)
      set_target_properties(SCR::SCR PROPERTIES
        IMPORTED_LOCATION             "${SCR_LIBRARY}"
        INTERFACE_LINK_LIBRARIES      "${SCR_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${SCR_INCLUDE_DIR}"
      )
    endif()
  endif()
endif()
