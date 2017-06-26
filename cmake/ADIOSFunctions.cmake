#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

function(adios_option name description default)
  set(ADIOS2_USE_${name} ${default} CACHE STRING "${description}")
  set_property(CACHE ADIOS2_USE_${name} PROPERTY
    STRINGS "ON;TRUE;AUTO;OFF;FALSE"
  )
endfunction()

function(message_pad msg out_len out_msg)
  string(LENGTH "${msg}" msg_len)
  if(NOT (msg_len LESS out_len))
    set(${out_msg} "${msg}" PARENT_SCOPE)
  else()
    math(EXPR pad_len "${out_len} - ${msg_len}")
    string(RANDOM LENGTH ${pad_len} pad)
    string(REGEX REPLACE "." " " pad "${pad}")
    set(${out_msg} "${msg}${pad}" PARENT_SCOPE)
  endif()
endfunction()

function(python_add_test name script)
  add_test(NAME ${name}
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${script} ${ARGN}
  )
  set_property(TEST ${name} PROPERTY
    ENVIRONMENT "PYTHONPATH=${ADIOS2_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}:$ENV{PYTHONPATH}"
  )
endfunction()

function(GenerateADIOSHeaderConfig)
  foreach(OPT IN LISTS ARGN)
    string(TOUPPER ${OPT} OPT_UPPER)
    if(ADIOS2_HAVE_${OPT})
      set(ADIOS2_HAVE_${OPT_UPPER} 1)
    else()
      set(ADIOS2_HAVE_${OPT_UPPER})
    endif()
  endforeach()

  configure_file(
    ${ADIOS2_SOURCE_DIR}/source/adios2/ADIOSConfig.h.in
    ${ADIOS2_BINARY_DIR}/source/adios2/ADIOSConfig.h
  )
endfunction()

function(GenerateADIOSPackageConfig)
  include(CMakePackageConfigHelpers)

  # Build interface configs
  write_basic_package_version_file(
    ${ADIOS2_BINARY_DIR}/ADIOS2ConfigVersion.cmake
    COMPATIBILITY AnyNewerVersion
  )
  export(EXPORT adios2Exports
    FILE ${ADIOS2_BINARY_DIR}/ADIOS2Targets.cmake
    NAMESPACE adios2::
  )
  configure_file(
    ${ADIOS2_SOURCE_DIR}/cmake/ADIOS2ConfigCommon.cmake.in
    ${ADIOS2_BINARY_DIR}/ADIOS2ConfigCommon.cmake
    @ONLY
  )
  configure_file(
    ${ADIOS2_SOURCE_DIR}/cmake/ADIOS2Config.cmake.in
    ${ADIOS2_BINARY_DIR}/ADIOS2Config.cmake
    @ONLY
  )

  # Install interface configs
  install(
    FILES
      ${ADIOS2_BINARY_DIR}/ADIOS2ConfigVersion.cmake
      ${ADIOS2_BINARY_DIR}/ADIOS2ConfigCommon.cmake
    DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
  )
  install(EXPORT adios2Exports
    FILE ADIOS2Targets.cmake
    NAMESPACE adios2::
    DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
  )
  configure_file(
    ${ADIOS2_SOURCE_DIR}/cmake/ADIOS2ConfigInstall.cmake.in
    ${ADIOS2_BINARY_DIR}/ADIOS2ConfigInstall.cmake
    @ONLY
  )
  install(FILES ${ADIOS2_BINARY_DIR}/ADIOS2ConfigInstall.cmake
    RENAME ADIOS2Config.cmake
    DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
  )

  # Install helper find modules if needed
  if(NOT BUILD_SHARED_LIBS)
    if(ADIOS2_HAVE_BZip2)
    install(FILES cmake/FindBZip2.cmake
      DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/Modules
    )
    install(FILES cmake/upstream/FindBZip2.cmake
      DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/Modules/upstream
    )
    endif()
    if(ADIOS2_HAVE_ZFP)
      install(FILES cmake/FindZFP.cmake
        DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/Modules
      )
    endif()
    if(ADIOS2_HAVE_ZeroMQ)
      install(FILES cmake/FindZeroMQ.cmake
        DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/Modules
      )
    endif()
    if(ADIOS2_HAVE_ADIOS1)
      install(FILES cmake/FindADIOS1.cmake
        DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/Modules
      )
    endif()
  endif()
endfunction()
