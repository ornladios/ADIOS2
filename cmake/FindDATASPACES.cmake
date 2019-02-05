#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindDATASPACES
# -----------
#
# Try to find the DataSpacfes library
#
# This module defines the following variables:
#
#   DATASPACES_FOUND        - System has DataSpaces
#   DATASPACES_INCLUDE_DIRS - The DataSpaces include directory
#   DATASPACES_LIBRARIES    - Link these to use DataSpaces
#
# and the following imported targets:
#   DataSpaces::DataSpaces - The DataSpaces compression library target
#
# You can also set the following variable to help guide the search:
#   DATASPACES_ROOT - The install prefix for DataSpaces containing the
#              include and lib folders
#              Note: this can be set as a CMake variable or an
#                    environment variable.  If specified as a CMake
#                    variable, it will override any setting specified
#                    as an environment variable.

if(NOT DATASPACES_FOUND)
  if((NOT DATASPACES_ROOT) AND (NOT (ENV{DATASPACES_ROOT} STREQUAL "")))
    set(DATASPACES_ROOT "$ENV{DATASPACES_ROOT}")
  endif()
  if(DATASPACES_ROOT)
    set(DATASPACES_INCLUDE_OPTS HINTS ${DATASPACES_ROOT}/include)
    set(DATASPACES_LIBRARY_OPTS
      HINTS ${DATASPACES_ROOT}/lib
    )
  endif()

  find_path(DATASPACES_INCLUDE_DIR dataspaces.h ${DATASPACES_INCLUDE_OPTS})
  find_library(DSPACESF_LIBRARY dspacesf ${DATASPACES_LIBRARY_OPTS})
  find_library(DSCOMMON_LIBRARY dscommon ${DATASPACES_LIBRARY_OPTS})
  find_library(DSPACES_LIBRARY dspaces ${DATASPACES_LIBRARY_OPTS}) 
  find_library(DART_LIBRARY dart ${DATASPACES_LIBRARY_OPTS})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(DATASPACES
    FOUND_VAR DATASPACES_FOUND
    REQUIRED_VARS DSPACESF_LIBRARY DSCOMMON_LIBRARY DART_LIBRARY DATASPACES_INCLUDE_DIR
  )
  if(DATASPACES_FOUND)
    set(DATASPACES_INCLUDE_DIRS ${DATASPACES_INCLUDE_DIR})
    set(DATASPACES_LIBRARIES ${DSPACES_LIBRARY} ${DSPACESF_LIBRARY} ${DSCOMMON_LIBRARY} ${DART_LIBRARY})
    if(DATASPACES_FOUND AND NOT TARGET DataSpaces::DataSpaces)
      add_library(DataSpaces::DataSpaces UNKNOWN IMPORTED)
      set_target_properties(DataSpaces::DataSpaces PROPERTIES
       	IMPORTED_LOCATION             "${DSPACES_LIBRARY}"
        INTERFACE_LINK_LIBRARIES      "${DATASPACES_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${DATASPACES_INCLUDE_DIR}"
      )
    endif()
  endif()
endif()
