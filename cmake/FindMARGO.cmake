# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

######################################################
# - Try to find Margo library
# Once done this will define
#  MARGO_FOUND - System has Margo
#  MARGO_INCLUDE_DIRS - The Margo include directories
#  MARGO_LIBRARIES - The libraries needed to use Margo
#  margo::margo - Imported target for Margo
#
######################################################

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  set(_MARGO_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
  if(MARGO_ROOT)
    list(INSERT CMAKE_PREFIX_PATH 0 "${MARGO_ROOT}")
  elseif(NOT ENV{MARGO_ROOT} STREQUAL "")
    list(INSERT CMAKE_PREFIX_PATH 0 "$ENV{MARGO_ROOT}")
  endif()
  set(PKG_CONFIG_USE_MARGO_CMAKE_PREFIX_PATH ON)

  pkg_check_modules(PC_MARGO margo)

  set(CMAKE_PREFIX_PATH ${_MARGO_CMAKE_PREFIX_PATH})
  unset(_MARGO_CMAKE_PREFIX_PATH)

  if(PC_MARGO_FOUND)
    if(BUILD_SHARED_LIBS)
      set(_PC_TYPE)
    else()
      set(_PC_TYPE _STATIC)
    endif()
    set(MARGO_INCLUDE_DIRS ${PC_MARGO${_PC_TYPE}_INCLUDE_DIRS})
    set(MARGO_LIBRARIES ${PC_MARGO${_PC_TYPE}_LINK_LIBRARIES})
    set(MARGO_DEFINITIONS ${PC_MARGO${_PC_TYPE}_CFLAGS_OTHER})
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MARGO DEFAULT_MSG MARGO_LIBRARIES)

if(MARGO_FOUND)
  if(NOT TARGET margo::margo)
    add_library(margo::margo INTERFACE IMPORTED)
    if(MARGO_INCLUDE_DIRS)
      set_target_properties(margo::margo PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MARGO_INCLUDE_DIRS}"
      )
    endif()
    if(MARGO_DEFINITIONS)
      set_target_properties(margo::margo PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${MARGO_DEFINITIONS}"
      )
    endif()
    if(MARGO_LIBRARIES)
      set_target_properties(margo::margo PROPERTIES
        INTERFACE_LINK_LIBRARIES "${MARGO_LIBRARIES}"
      )
    endif()
  endif()
endif()

mark_as_advanced(MARGO_INCLUDE_DIRS MARGO_LIBRARIES)
