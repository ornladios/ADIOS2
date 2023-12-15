#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if (NOT ADIOS2_USE_PIP)
  add_executable(remote_server ./remote_server.cpp remote_common.cpp)

  target_link_libraries(remote_server PUBLIC EVPath::EVPath adios2_core adios2sys
    PRIVATE $<$<PLATFORM_ID:Windows>:shlwapi>)

  get_property(pugixml_headers_path
    TARGET pugixml
    PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  )

  target_include_directories(remote_server PRIVATE ${PROJECT_BINARY_DIR} ${pugixml_headers_path})

  set_property(TARGET remote_server PROPERTY OUTPUT_NAME remote_server${ADIOS2_EXECUTABLE_SUFFIX})
  install(TARGETS remote_server EXPORT adios2
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT adios2_tools-runtime
  )
endif ()
