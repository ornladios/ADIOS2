#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

function(GenerateADIOSConfig)
  foreach(OPT IN LISTS ARGN)
    string(TOUPPER ${OPT} OPT_UPPER)
    if(ADIOS_USE_${OPT})
      set(ADIOS2_HAVE_${OPT_UPPER} 1)
    else()
      set(ADIOS2_HAVE_${OPT_UPPER})
    endif()
  endforeach()

  configure_file(
    ${ADIOS_SOURCE_DIR}/source/adios2/ADIOSConfig.h.in
    ${ADIOS_BINARY_DIR}/adios2/ADIOSConfig.h
  )
endfunction()
