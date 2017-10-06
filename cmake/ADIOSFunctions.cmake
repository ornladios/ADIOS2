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

function(python_add_test)
  set(options)
  # NAME: test name
  # ADDTOPYPATH: Provide this if python modules need to be found in specific directories
  # USE_MPI_FOR_PYTHON_TESTS: Set to anything for mpi mode
  set(oneValueArgs
      NAME
      ADDTOPYPATH
      USE_MPI_FOR_PYTHON_TESTS
  )
  # EXEC_WRAPPER: Any extra arguments to pass on the command line before test case
  # SCRIPT: Script name and corresponding comand line inputs
  set(multiValueArgs EXEC_WRAPPER SCRIPT)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")
  add_test(NAME ${ARGS_NAME}
    COMMAND ${ARGS_EXEC_WRAPPER} ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${ARGS_SCRIPT}
  )

  set(testsPythonPath "${ADIOS2_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}:$ENV{PYTHONPATH}")

  if(DEFINED ARGS_ADDTOPYPATH)
    set(testsPythonPath "${ARGS_ADDTOPYPATH}:${testsPythonPath}")
  endif()

  if(ARGS_USE_MPI_FOR_PYTHON_TESTS)
    set(mpi_flag "ADIOS2_PYTHON_TESTS_USE_MPI=${ARGS_USE_MPI_FOR_PYTHON_TESTS}")
  endif()

  set_property(TEST ${ARGS_NAME} PROPERTY ENVIRONMENT
    "PYTHONPATH=${testsPythonPath}"
    ${mpi_flag}
  )
endfunction()

function(GenerateADIOSHeaderConfig)
  set(ADIOS2_CONFIG_DEFINES)
  foreach(OPT IN LISTS ARGN)
    string(TOUPPER ${OPT} OPT_UPPER)
    string(APPEND ADIOS2_CONFIG_DEFINES "
/* CMake Option: ADIOS_USE_${OPT}=OFF */
#cmakedefine ADIOS2_HAVE_${OPT_UPPER}
")
    if(ADIOS2_HAVE_${OPT})
      set(ADIOS2_HAVE_${OPT_UPPER} 1)
    else()
      set(ADIOS2_HAVE_${OPT_UPPER})
    endif()
  endforeach()

  configure_file(
    ${ADIOS2_SOURCE_DIR}/source/adios2/ADIOSConfig.h.in
    ${ADIOS2_BINARY_DIR}/source/adios2/ADIOSConfig.h.in
  )
  configure_file(
    ${ADIOS2_BINARY_DIR}/source/adios2/ADIOSConfig.h.in
    ${ADIOS2_BINARY_DIR}/source/adios2/ADIOSConfig.h
  )
endfunction()

macro(__adios2_list_cleanup_for_bash var)
  if(${var})
    list(REMOVE_DUPLICATES ${var})
  endif()
  string(REPLACE ";" " " ${var} "${${var}}")
endmacro()

function(__adios2_list_make_link_args var)
  set(prefixes)
  foreach(lib IN LISTS ${var})
    if(lib MATCHES "^/")
      get_filename_component(lib_dir "${lib}" DIRECTORY)
      list(APPEND prefixes "${lib_dir}")
    endif()
  endforeach()

  set(var_new)
  foreach(prefix IN LISTS prefixes)
    list(APPEND var_new "-L${prefix}")
  endforeach()
  foreach(lib IN LISTS ${var})
    if(lib MATCHES "^/.*/?(${CMAKE_SHARED_LIBRARY_PREFIX}|${CMAKE_STATIC_LIBRARY_PREFIX})(.*)(${CMAKE_SHARED_LIBRARY_SUFFIX}|${CMAKE_STATIC_LIBRARY_SUFFIX})")
      list(APPEND var_new "-l${CMAKE_MATCH_2}")
    else()
      list(APPEND var_new "${lib}")
    endif()
  endforeach()

  set(${var} ${var_new} PARENT_SCOPE)
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
  # Also build flags needed for non-cmake config generation
  set(ADIOS2_CXX_LIBS ${CMAKE_THREAD_LIBS_INIT})
  set(ADIOS2_CXX_OPTS ${CMAKE_CXX11_EXTENSION_COMPILE_OPTION})
  set(ADIOS2_CXX_DEFS)
  set(ADIOS2_CXX_INCS)
  if(ADIOS2_HAVE_MPI)
    list(APPEND ADIOS2_CXX_LIBS ${MPI_C_LIBRARIES})
    list(APPEND ADIOS2_CXX_INCS ${MPI_C_INCLUDE_PATH})
  endif()
  if(NOT BUILD_SHARED_LIBS)
    if(ADIOS2_HAVE_DataMan)
    list(APPEND ADIOS2_CXX_LIBS -ldataman)
    endif()
    if(ADIOS2_HAVE_BZip2)
      install(FILES cmake/FindBZip2.cmake
        DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/Modules
      )
      install(FILES cmake/upstream/FindBZip2.cmake
        DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/Modules/upstream
      )
      list(APPEND ADIOS2_CXX_LIBS ${BZIP2_LIBRARIES})
      list(APPEND ADIOS2_CXX_INCS ${BZIP2_INCLUDE_DIR})
    endif()
    if(ADIOS2_HAVE_ZFP)
      install(FILES cmake/FindZFP.cmake
        DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/Modules
      )
      list(APPEND ADIOS2_CXX_LIBS ${ZFP_LIBRARIES})
      list(APPEND ADIOS2_CXX_INCS ${ZFP_INCLUDE_DIRS})
    endif()
    if(ADIOS2_HAVE_ZeroMQ)
      install(FILES cmake/FindZeroMQ.cmake
        DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/Modules
      )
      list(APPEND ADIOS2_CXX_LIBS ${ZeroMQ_LIBRARIES})
      list(APPEND ADIOS2_CXX_INCS ${ZeroMQ_INCLUDE_DIRS})
    endif()
    if(ADIOS2_HAVE_ADIOS1)
      install(FILES cmake/FindADIOS1.cmake
        DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/Modules
      )
      list(APPEND ADIOS2_CXX_LIBS ${ADIOS1_LIBRARIES})
      list(APPEND ADIOS2_CXX_INCS ${ADIOS1_INCLUDE_DIRS})
    endif()
    if(ADIOS2_HAVE_HDF5)
      list(APPEND ADIOS2_CXX_LIBS ${HDF5_C_LIBRARIES})
      if(HDF5_C_INCLUDE_DIRS)
        list(APPEND ADIOS2_CXX_INCS ${HDF5_C_INCLUDE_DIRS})
      else()
        list(APPEND ADIOS2_CXX_INCS ${HDF5_INCLUDE_DIRS})
      endif()
    endif()
  endif()

  # Build the non-cmake config script
  __adios2_list_make_link_args(ADIOS2_CXX_LIBS)
  __adios2_list_cleanup_for_bash(ADIOS2_CXX_LIBS)
  __adios2_list_cleanup_for_bash(ADIOS2_CXX_OPTS)
  __adios2_list_cleanup_for_bash(ADIOS2_CXX_DEFS)
  __adios2_list_cleanup_for_bash(ADIOS2_CXX_INCS)
  configure_file(
    ${ADIOS2_SOURCE_DIR}/cmake/adios2-config.in
    ${ADIOS2_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/adios2-config
    @ONLY
  )
  install(PROGRAMS ${ADIOS2_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/adios2-config
    DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endfunction()
