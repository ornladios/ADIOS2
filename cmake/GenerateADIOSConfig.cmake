#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

function(GenerateADIOSConfig)
  configure_file(
    ${ADIOS_SOURCE_DIR}/source/adios2/ADIOSConfig.h.in
    ${ADIOS_BINARY_DIR}/adios2/ADIOSConfig.h
  )

  foreach(L IN ITEMS C CXX Fortran)
    if(NOT CMAKE_${L}_COMPILER)
      continue()
    endif()
    file(APPEND ${ADIOS_BINARY_DIR}/adios2/ADIOSConfig.h
      " *   ${L}: ${CMAKE_${L}_COMPILER}\n"
    )
    if(CMAKE_${L}_COMPILER_WRAPPER)
      file(APPEND ${ADIOS_BINARY_DIR}/adios2/ADIOSConfig.h
        " *    Wrapper: ${CMAKE_${L}_COMPILER_WRAPPER}\n"
      )
    endif()
    file(APPEND ${ADIOS_BINARY_DIR}/adios2/ADIOSConfig.h
      " *    Id: ${CMAKE_${L}_COMPILER_ID} ${CMAKE_${L}_COMPILER_VERSION}\n"
    )
  endforeach()
  file(APPEND ${ADIOS_BINARY_DIR}/adios2/ADIOSConfig.h " */\n")

  foreach(OPT IN LISTS ARGN)
    string(TOUPPER ${OPT} OPT_UPPER)
    file(APPEND ${ADIOS_BINARY_DIR}/adios2/ADIOSConfig.h
      "\n/* CMake Option: ADIOS_USE_${OPT}=${ADIOS_USE_${OPT}} */\n"
    )
    if(ADIOS_USE_${OPT})
      file(APPEND ${ADIOS_BINARY_DIR}/adios2/ADIOSConfig.h
        "#define ADIOS2_HAVE_${OPT_UPPER} 1\n"
      )
    else()
      file(APPEND ${ADIOS_BINARY_DIR}/adios2/ADIOSConfig.h
        "#undef ADIOS2_HAVE_${OPT_UPPER}\n"
      )
    endif()
  endforeach()
  file(APPEND ${ADIOS_BINARY_DIR}/adios2/ADIOSConfig.h
    "\n#endif /* ADIOSCONFIG_H_ */\n"
  )
endfunction()
