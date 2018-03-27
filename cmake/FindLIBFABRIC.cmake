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

find_path(LIBFABRIC_INCLUDE_DIR rdma/fabric.h)
find_library(LIBFABRIC_LIBRARY fabric)
mark_as_advanced(LIBFABRIC_INCLUDE_DIR LIBFABRIC_LIBRARY)

if(LIBFABRIC_PREFIX)
  set(CMAKE_PREFIX_PATH ${_CMAKE_PREFIX_PATH})
  unset(_CMAKE_PREFIX_PATH)
endif()

######################################################
include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBFABRIC_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(libfabric  DEFAULT_MSG
    LIBFABRIC_LIBRARY LIBFABRIC_INCLUDE_DIR)

if(LIBFABRIC_FOUND)
  set(LIBFABRIC_LIBRARIES ${LIBFABRIC_LIBRARY})
  set(LIBFABRIC_INCLUDE_DIRS ${LIBFABRIC_INCLUDE_DIR})
  if(NOT TARGET libfabric::libfabric)
    add_library(libfabric::libfabric UNKNOWN IMPORTED)
    set_target_properties(libfabric::libfabric PROPERTIES
      IMPORTED_LOCATION "${LIBFABRIC_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${LIBFABRIC_INCLUDE_DIR}")
  endif()
endif()
