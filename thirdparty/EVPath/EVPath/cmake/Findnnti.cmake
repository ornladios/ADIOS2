######################################################
# Once done this will define
#  NNTI_FOUND - System has libfabric
#  NNTI_INCLUDE_DIRS - The libfabric include directories
#  NNTI_LIBRARIES - The libraries needed to use libfabric
#  NNTI_DEFINITIONS - Compiler switches required for using libfabric

######################################################
set(NNTI_NNTI "" CACHE STRING "Help cmake to find nnti library on your system.")

######################################################
find_path(NNTI_INCLUDE_DIR  Trios_nnti.h
    HINTS ${NNTI_DIR}/include)

######################################################
find_library(NNTI_LIBRARY NAMES trios_nnti
    HINTS ${NNTI_DIR}/lib)

######################################################
set(NNTI_LIBRARIES ${NNTI_LIBRARY} )
set(NNTI_INCLUDE_DIRS ${NNTI_INCLUDE_DIR} )

######################################################
include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set NNTI_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(nnti  DEFAULT_MSG
    NNTI_LIBRARY NNTI_INCLUDE_DIR)

######################################################
mark_as_advanced(NNTI_INCLUDE_DIR NNTI_LIBRARY )
