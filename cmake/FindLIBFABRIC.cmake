######################################################
# - Try to find libfabric (http://directory.fsf.org/wiki/Libfabric)
# Once done this will define
#  LIBFABRIC_FOUND - System has libfabric
#  LIBFABRIC_INCLUDE_DIRS - The libfabric include directories
#  LIBFABRIC_LIBRARIES - The libraries needed to use libfabric

######################################################
set(LIBFABRIC_PREFIX "" CACHE STRING "Help cmake to find libfabric library (https://github.com/ofiwg/libfabric) into your system.")
mark_as_advanced(LIBFABRIC_PREFIX)
if(NOT LIBFABRIC_PREFIX)
  set(LIBFABRIC_PREFIX ${LIBFABRIC_ROOT})
endif()
if(NOT LIBFABRIC_PREFIX)
  set(LIBFABRIC_PREFIX $ENV{LIBFABRIC_ROOT})
endif()

if(LIBFABRIC_PREFIX)
  set(_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
  list(INSERT CMAKE_PREFIX_PATH 0 ${LIBFABRIC_PREFIX})
endif()

include(CMakeFindDependencyMacro)
find_dependency(PkgConfig)
if(PKG_CONFIG_FOUND)
  set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)
  pkg_check_modules(LIBFABRIC IMPORTED_TARGET libfabric)
endif()

if(LIBFABRIC_FOUND)
  if(NOT TARGET libfabric::libfabric)
    add_library(libfabric::libfabric INTERFACE IMPORTED)
    set_target_properties(libfabric::libfabric PROPERTIES
      INTERFACE_LINK_LIBRARIES PkgConfig::LIBFABRIC)
  endif()
endif()
