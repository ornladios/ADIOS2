######################################################
# - Try to find libfabric (http://directory.fsf.org/wiki/Libfabric)
# Once done this will define
#  LIBFABRIC_FOUND - System has libfabric
#  LIBFABRIC_INCLUDE_DIRS - The libfabric include directories
#  LIBFABRIC_LIBRARIES - The libraries needed to use libfabric
#  LIBFABRIC_DEFINITIONS - Compiler switches required for using libfabric

######################################################
set(LIBFABRIC_PREFIX "" CACHE STRING "Help cmake to find libfabric library (https://github.com/ofiwg/libfabric) into your system.")

######################################################
find_path(LIBFABRIC_INCLUDE_DIR rdma/fabric.h
    HINTS ${LIBFABRIC_PREFIX}/include)

######################################################
find_library(LIBFABRIC_LIBRARY NAMES fabric
    HINTS ${LIBFABRIC_PREFIX}/lib)

######################################################
set(LIBFABRIC_LIBRARIES ${LIBFABRIC_LIBRARY} )
set(LIBFABRIC_INCLUDE_DIRS ${LIBFABRIC_INCLUDE_DIR} )

######################################################
include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBFABRIC_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(libfabric  DEFAULT_MSG
    LIBFABRIC_LIBRARY LIBFABRIC_INCLUDE_DIR)

######################################################
mark_as_advanced(LIBFABRIC_INCLUDE_DIR LIBFABRIC_LIBRARY )

