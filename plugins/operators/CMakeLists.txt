# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_Sodium)
  add_library(EncryptionOperator MODULE
    EncryptionOperator.cpp
  )
  target_link_libraries(EncryptionOperator sodium)
  target_include_directories(EncryptionOperator PRIVATE
    ${PROJECT_SOURCE_DIR}/source
    ${PROJECT_BINARY_DIR}/source
  )
  if (NOT CMAKE_SKIP_INSTALL_RPATH)
    set_target_properties(EncryptionOperator PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
  endif()
  install(TARGETS EncryptionOperator  EXPORT adios2Exports
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT adios2_core-runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT adios2_core-libraries NAMELINK_COMPONENT adios2_core-development
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT adios2_core-development
)

  if(ADIOS2_HAVE_SealKeygen)
    install(PROGRAMS adios2_seal_keygen.py
      DESTINATION ${CMAKE_INSTALL_BINDIR}
      RENAME adios2_seal_keygen
      COMPONENT adios2_core-runtime
    )
  endif()
endif()
