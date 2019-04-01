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
  set(oneValueArgs
      NAME
  )
  # EXEC_WRAPPER: Any extra arguments to pass on the command line before test case
  # SCRIPT: Script name and corresponding comand line inputs
  set(multiValueArgs EXEC_WRAPPER SCRIPT)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")
  add_test(NAME ${ARGS_NAME}
    COMMAND ${ARGS_EXEC_WRAPPER} ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${ARGS_SCRIPT}
  )
  set_property(TEST ${ARGS_NAME} PROPERTY
    ENVIRONMENT "PYTHONPATH=${ADIOS2_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}:$ENV{PYTHONPATH}"
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


function(adios2_add_thirdparty_target PackageName TargetName)
  find_package(${PackageName} REQUIRED)
  add_library(adios2::thirdparty::${PackageName} INTERFACE IMPORTED GLOBAL)
  target_link_libraries(adios2::thirdparty::${PackageName}
    INTERFACE ${TargetName}
  )
endfunction()
