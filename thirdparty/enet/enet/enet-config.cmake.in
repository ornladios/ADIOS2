# - Config file for the ENET package
#
# It defines the following variables
#   ENET_INCLUDE_DIRS - include directories for Dill
#   ENET_LIBRARIES    - libraries to link against
#
# And the folloing imported targets:
#   enet::enet
#
 
include("${CMAKE_CURRENT_LIST_DIR}/enet-config-version.cmake")

include(FindPackageHandleStandardArgs)
set(${CMAKE_FIND_PACKAGE_NAME}_CONFIG "${CMAKE_CURRENT_LIST_FILE}")
find_package_handle_standard_args(${CMAKE_FIND_PACKAGE_NAME} CONFIG_MODE)

if(NOT TARGET enet::enet)
  include("${CMAKE_CURRENT_LIST_DIR}/enet-targets.cmake")
endif()

set(ENET_LIBRARIES enet::enet)
set(ENET_INCLUDE_DIRS
  $<TARGET_PROPERTY:enet::enet,INTERFACE_INCLUDE_DIRECTORIES>
)
