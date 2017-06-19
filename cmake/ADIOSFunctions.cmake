#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

function(GenerateADIOSConfig)
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
    ${ADIOS2_BINARY_DIR}/adios2/ADIOSConfig.h
  )
endfunction()

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
