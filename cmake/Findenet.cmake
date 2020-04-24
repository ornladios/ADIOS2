#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

find_package(enet QUIET NO_MODULE)
if(enet_FOUND)
  find_package_handle_standard_args(enet CONFIG_MODE)
  return()
endif()

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(PC_ENET QUIET libenet)
endif()

find_package_handle_standard_args(enet
  REQUIRED_VARS PC_ENET_LINK_LIBRARIES
  VERSION_VAR PC_ENET_VERSION)

if(enet_FOUND)
  if(NOT TARGET enet::enet)
    add_library(enet::enet IMPORTED UNKNOWN)
    set_target_properties(enet::enet PROPERTIES
      IMPORTED_LOCATION "${PC_ENET_LINK_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${PC_ENET_INCLUDE_DIRS}")
  endif()
endif()
