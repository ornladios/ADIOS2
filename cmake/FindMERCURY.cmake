# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

######################################################
# - Try to find Mercury RPC library
# Once done this will define
#  MERCURY_FOUND - System has Mercury
#  MERCURY_INCLUDE_DIRS - The Mercury include directories
#  MERCURY_LIBRARIES - The libraries needed to use Mercury
#  mercury::mercury - Imported target for Mercury
#
######################################################

# Try to find Mercury using its CMake config first
find_package(mercury ${MERCURY_FIND_VERSION} QUIET CONFIG
  HINTS
    ${MERCURY_ROOT}
    $ENV{MERCURY_ROOT}
  PATH_SUFFIXES
    share/cmake/mercury
    lib/cmake/mercury
)

if(mercury_FOUND)
  set(MERCURY_FOUND TRUE)

  # Get include dirs and libraries from the imported target
  if(TARGET mercury)
    get_target_property(MERCURY_INCLUDE_DIRS mercury INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(MERCURY_LIBRARIES mercury INTERFACE_LINK_LIBRARIES)

    # Create alias with mercury:: namespace if it doesn't exist
    if(NOT TARGET mercury::mercury)
      add_library(mercury::mercury ALIAS mercury)
    endif()
  endif()
else()
  # Fallback to manual search
  find_path(MERCURY_INCLUDE_DIR
    NAMES mercury.h
    HINTS
      ${MERCURY_ROOT}
      $ENV{MERCURY_ROOT}
    PATH_SUFFIXES include
  )

  find_library(MERCURY_LIBRARY
    NAMES mercury
    HINTS
      ${MERCURY_ROOT}
      $ENV{MERCURY_ROOT}
    PATH_SUFFIXES lib lib64
  )

  set(MERCURY_INCLUDE_DIRS ${MERCURY_INCLUDE_DIR})
  set(MERCURY_LIBRARIES ${MERCURY_LIBRARY})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(MERCURY
    REQUIRED_VARS MERCURY_LIBRARY MERCURY_INCLUDE_DIR
    VERSION_VAR MERCURY_VERSION
  )

  if(MERCURY_FOUND AND NOT TARGET mercury::mercury)
    add_library(mercury::mercury INTERFACE IMPORTED)
    set_target_properties(mercury::mercury PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${MERCURY_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${MERCURY_LIBRARIES}"
    )
  endif()
endif()

mark_as_advanced(MERCURY_INCLUDE_DIR MERCURY_LIBRARY)
