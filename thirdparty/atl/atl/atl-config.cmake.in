# - Config file for the Atl package
#
# It defines the following variables
#  ATL_INCLUDE_DIRS - include directories for Atl
#  ATL_LIBRARIES    - libraries to link against
#
# And the following imported targets:
#   atl::atl
#

include("${CMAKE_CURRENT_LIST_DIR}/atl-config-version.cmake")

include(FindPackageHandleStandardArgs)
set(${CMAKE_FIND_PACKAGE_NAME}_CONFIG "${CMAKE_CURRENT_LIST_FILE}")
find_package_handle_standard_args(${CMAKE_FIND_PACKAGE_NAME} CONFIG_MODE)

if(NOT TARGET atl::atl)
  include("${CMAKE_CURRENT_LIST_DIR}/atl-targets.cmake")
endif()

set(ATL_LIBRARIES atl::atl)
set(ATL_INCLUDE_DIRS
  $<TARGET_PROPERTY:atl::atl,INTERFACE_INCLUDE_DIRECTORIES>
)
