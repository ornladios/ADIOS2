######################################################
# - Try to find libfabric (http://directory.fsf.org/wiki/Libfabric)
# Once done this will define
#  LIBFABRIC_FOUND - System has libfabric
#  LIBFABRIC_INCLUDE_DIRS - The libfabric include directories
#  LIBFABRIC_LIBRARIES - The libraries needed to use libfabric

######################################################
set(LIBFABRIC_ROOT "" CACHE STRING "Help cmake to find libfabric library (https://github.com/ofiwg/libfabric) into your system.")
mark_as_advanced(LIBFABRIC_ROOT)
if(NOT LIBFABRIC_ROOT AND "$ENV{LIBFABRIC_ROOT}" STREQUAL "")
  set(LIBFABRIC_ROOT ${LIBFABRIC_ROOT})
endif()

unset(_CMAKE_PREFIX_PATH)
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
else()
  if(NOT LIBFABRIC_ROOT)
    set(LIBFABRIC_ROOT "$ENV{LIBFABRIC_ROOT}")
  endif()
  if(LIBFABRIC_ROOT)
    set(_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
    list(INSERT CMAKE_PREFIX_PATH 0 ${LIBFABRIC_ROOT})
  endif()
endif()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)
  if(LIBFABRIC_FIND_VERSION)
    if(LIBFABRIC_VERSION_EXACT)
      set(_pkg_ver_arg "=${LIBFABRIC_FIND_VERSION}")
    else()
      set(_pkg_ver_arg ">=${LIBFABRIC_FIND_VERSION}")
    endif()
  endif()
  pkg_check_modules(LIBFABRIC IMPORTED_TARGET "libfabric${_pkg_ver_arg}")
endif()

if(_CMAKE_PREFIX_PATH)
  set(CMAKE_PREFIX_PATH ${_CMAKE_PREFIX_PATH})
  unset(_CMAKE_PREFIX_PATH)
endif()

if(LIBFABRIC_FOUND)
  if(NOT TARGET libfabric::libfabric)
    add_library(libfabric::libfabric INTERFACE IMPORTED)
    if(NOT BUILD_SHARED_LIBS AND TARGET PkgConfig::LIBFABRIC-static)
      set_target_properties(libfabric::libfabric PROPERTIES
        INTERFACE_LINK_LIBRARIES PkgConfig::LIBFABRIC-static)
    else()
      set_target_properties(libfabric::libfabric PROPERTIES
        INTERFACE_LINK_LIBRARIES PkgConfig::LIBFABRIC)
    endif()
  endif()
endif()
