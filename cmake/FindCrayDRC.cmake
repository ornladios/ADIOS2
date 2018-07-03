######################################################
# - Try to find Cray DRC library
# Once done this will define
#  CRAY_DRC_FOUND - System has Cray DRC
#  CRAY_DRC_INCLUDE_DIRS - The Cray DRC include directories
#  CRAY_DRC_LIBRARIES - The libraries needed to use Cray DRC

######################################################
set(CRAY_DRC_PREFIX "" CACHE STRING "Help cmake to find Cray DRC library on your system.")
mark_as_advanced(CRAY_DRC_PREFIX)
if(NOT CRAY_DRC_ROOT AND "$ENV{CRAY_DRC_ROOT}" STREQUAL "")
  set(CRAY_DRC_ROOT ${CRAY_DRC_PREFIX})
endif()

unset(_CMAKE_PREFIX_PATH)
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
else()
  if(NOT CRAY_DRC_ROOT)
    set(CRAY_DRC_ROOT "$ENV{CRAY_DRC_ROOT}")
  endif()
  if(CRAY_DRC_ROOT)
    set(_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
    list(INSERT CMAKE_PREFIX_PATH 0 ${CRAY_DRC_ROOT})
  endif()
endif()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)
  pkg_check_modules(CRAY_DRC IMPORTED_TARGET "cray-drc")
endif()

if(_CMAKE_PREFIX_PATH)
  set(CMAKE_PREFIX_PATH ${_CMAKE_PREFIX_PATH})
  unset(_CMAKE_PREFIX_PATH)
endif()

if(CRAY_DRC_FOUND)
  if(NOT TARGET craydrc::craydrc)
    add_library(craydrc::craydrc INTERFACE IMPORTED)
    if(NOT BUILD_SHARED_LIBS AND TARGET PkgConfig::CRAY_DRC-static)
      set_target_properties(craydrc::craydrc PROPERTIES
        INTERFACE_LINK_LIBRARIES PkgConfig::CRAY_DRC-static)
    else()
      set_target_properties(craydrc::craydrc PROPERTIES
        INTERFACE_LINK_LIBRARIES PkgConfig::CRAY_DRC)
    endif()
  endif()
endif()
