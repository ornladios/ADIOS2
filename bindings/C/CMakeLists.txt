#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#


target_sources(adios2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/adios2_c.cpp)
target_include_directories(adios2
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

install(
  FILES adios2_c.h 
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(
  FILES adios2/adios2_c_enums.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2
)
