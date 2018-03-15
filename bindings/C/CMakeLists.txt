#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#


target_sources(adios2 PRIVATE 
${CMAKE_CURRENT_SOURCE_DIR}/adios2/adios2_c_adios.cpp
${CMAKE_CURRENT_SOURCE_DIR}/adios2/adios2_c_glue.cpp
${CMAKE_CURRENT_SOURCE_DIR}/adios2/adios2_c_io.cpp
${CMAKE_CURRENT_SOURCE_DIR}/adios2/adios2_c_engine.cpp
${CMAKE_CURRENT_SOURCE_DIR}/adios2/adios2_c_variable.cpp
${CMAKE_CURRENT_SOURCE_DIR}/adios2/adios2_c_FILE.cpp
)

target_include_directories(adios2
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

install(
  FILES adios2_c.h 
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  FILES adios2/adios2_c_types.h 
        adios2/adios2_c_adios.h
        adios2/adios2_c_glue.h 
        adios2/adios2_c_io.h
        adios2/adios2_c_variable.h
        adios2/adios2_c_engine.h
        adios2/adios2_c_FILE.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2
)
