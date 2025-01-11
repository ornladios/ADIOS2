#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_Sodium)
  add_library(adios2_encryption_operator MODULE
    EncryptionOperator.cpp
  )
  target_link_libraries(adios2_encryption_operator adios2_core sodium)
  if (NOT CMAKE_SKIP_INSTALL_RPATH)
    set_target_properties(adios2_encryption_operator PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
  endif()

  install(TARGETS adios2_encryption_operator  EXPORT adios2Exports
    RUNTIME DESTINATION ${ADIOS2_INSTALL_PLUGIN_BINDIR} COMPONENT adios2_core-runtime
    LIBRARY DESTINATION ${ADIOS2_INSTALL_PLUGIN_LIBDIR} COMPONENT adios2_core-libraries NAMELINK_COMPONENT adios2_core-development
  )
endif()
