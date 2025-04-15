#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindCaliper
# -----------
#
# Try to find the Caliper library
#
# This module defines the following variables:
#
#   Caliper_FOUND        - System has Caliper
#   Caliper_INCLUDE_DIRS - The Caliper include directory
#   Caliper_LIBRARIES    - Link these to use Caliper
#
# and the following imported targets:
#   Caliper::Caliper - The core Caliper library
#
# You can also set the following variable to help guide the search:
#   Caliper_ROOT - The install prefix for Caliper containing the
#                     include and lib folders
#                     Note: this can be set as a CMake variable or an
#                           environment variable.  If specified as a CMake
#                           variable, it will override any setting specified
#                           as an environment variable.

if((NOT Caliper_ROOT) AND (NOT (ENV{Caliper_ROOT} STREQUAL "")))
  set(Caliper_ROOT "$ENV{Caliper_ROOT}")
endif()
if(Caliper_ROOT)
  set(Caliper_INCLUDE_OPTS HINTS ${Caliper_ROOT}/include NO_DEFAULT_PATHS)
  set(Caliper_LIBRARY_OPTS
    HINTS ${Caliper_ROOT}/lib ${Caliper_ROOT}/lib64
    NO_DEFAULT_PATHS
    )
endif()

find_path(Caliper_INCLUDE_DIR caliper/cali.h ${Caliper_INCLUDE_OPTS})
find_library(Caliper_LIBRARY libcaliper.so ${Caliper_LIBRARY_OPTS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Caliper
  FOUND_VAR Caliper_FOUND
  REQUIRED_VARS Caliper_LIBRARY Caliper_INCLUDE_DIR
)

if(Caliper_FOUND)
  set(Caliper_INCLUDE_DIRS ${Caliper_INCLUDE_DIR})
  set(Caliper_LIBRARIES ${Caliper_LIBRARY})
  message(STATUS "Caliper Libraries \"${Caliper_LIBRARIES}\"")
  if(NOT TARGET Caliper::Caliper)
    add_library(Caliper::Caliper UNKNOWN IMPORTED)
    set_target_properties(Caliper::Caliper PROPERTIES
      IMPORTED_LOCATION             "${Caliper_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${Caliper_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES      "${Caliper_LIBRARIES}"
    )
  endif()
endif()
