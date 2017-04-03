#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindDataMan
# ---------
#
# Try to find the DataMan library from Jason Wang, ORNL
# https://github.com/JasonRuonanWang/DataMan
#
# This module defines the following variables:
#
#   DataMan_FOUND        - System has DataMan
#   DataMan_INCLUDE_DIRS - The DataMan include directory
#   DataMan_LIBRARIES    - Link these to use DataMan
#
# and the following imported targets:
#   DataMan::DataMan - The core DataMan library
#
# You can also set the following variable to help guide the search:
#   DataMan_ROOT_DIR - The install prefix for DataMan containing the
#                      include and lib folders
#                      Note: this can be set as a CMake variable or an
#                            environment variable.  If specified as a CMake
#                            variable, it will override any setting specified
#                            as an environment variable.

if(NOT DataMan_FOUND)
  if((NOT DataMan_ROOT_DIR) AND (NOT (ENV{DataMan_ROOT_DIR} STREQUAL "")))
    set(DataMan_ROOT_DIR "$ENV{DataMan_ROOT_DIR}")
  endif()

  # Search for the core libraries
  if(DataMan_ROOT_DIR)
    # If a root directory is specified, then don't look anywhere else 
    find_path(DataMan_INCLUDE_DIR DataMan.h
      HINTS ${DataMan_ROOT_DIR}/include
      NO_DEFAULT_PATHS
    )
    set(_DataMan_LIBRARY_HINT HINTS ${DataMan_ROOT_DIR}/lib NO_DEFAULT_PATHS)
  else()
    # Otherwise use the include dir as a basis to search for the lib
    find_path(DataMan_INCLUDE_DIR DataMan.h)
    if(DataMan_INCLUDE_DIR)
      get_filename_component(_DataMan_PREFIX "${DataMan_INCLUDE_DIR}" PATH)
      set(_DataMan_LIBRARY_HINT HINTS ${_DataMan_PREFIX}/lib)
      unset(_DataMan_PREFIX)
    endif()
  endif()
  find_library(DataMan_LIBRARY dataman ${_DataMan_LIBRARY_HINT})
  unset(_DataMan_LIBRARY_HINT)

  find_package_handle_standard_args(DataMan
    FOUND_VAR DataMan_FOUND
    REQUIRED_VARS
      DataMan_INCLUDE_DIR
      DataMan_LIBRARY
  )
  if(DataMan_FOUND)
    set(DataMan_INCLUDE_DIRS ${DataMan_INCLUDE_DIR})
    set(DataMan_LIBRARIES ${DataMan_LIBRARY})
    if(DataMan_FOUND AND NOT TARGET DataMan::DataMan)
      add_library(DataMan::DataMan UNKNOWN IMPORTED)
      set_target_properties(DataMan::DataMan PROPERTIES
        IMPORTED_LOCATION "${DataMan_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${DataMan_INCLUDE_DIR}"
      )
    endif()
  endif()
endif()
