#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#


target_sources(adios2 PRIVATE 
${CMAKE_CURRENT_SOURCE_DIR}/adios2/c/adios2_c_adios.cpp
${CMAKE_CURRENT_SOURCE_DIR}/adios2/c/adios2_c_io.cpp
${CMAKE_CURRENT_SOURCE_DIR}/adios2/c/adios2_c_engine.cpp
${CMAKE_CURRENT_SOURCE_DIR}/adios2/c/adios2_c_variable.cpp
${CMAKE_CURRENT_SOURCE_DIR}/adios2/c/adios2_c_attribute.cpp
${CMAKE_CURRENT_SOURCE_DIR}/adios2/c/adios2_c_operator.cpp
)
target_include_directories(adios2
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
if(NOT MSVC)
  target_compile_features(adios2 PRIVATE ${ADIOS2_C99_FEATURES})
endif()

install(
  FILES adios2_c.h 
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  FILES adios2/c/adios2_c_types.h 
        adios2/c/adios2_c_adios.h
        adios2/c/adios2_c_io.h
        adios2/c/adios2_c_variable.h
        adios2/c/adios2_c_attribute.h
        adios2/c/adios2_c_engine.h
        adios2/c/adios2_c_operator.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2/c
)
