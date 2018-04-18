#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_library(adios2_cxx03
	${CMAKE_CURRENT_SOURCE_DIR}/cxx03/cxx03ADIOS.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/cxx03/cxx03Attribute.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/cxx03/cxx03Attribute.tcc
	${CMAKE_CURRENT_SOURCE_DIR}/cxx03/cxx03Engine.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/cxx03/cxx03IO.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/cxx03/cxx03IO.tcc
	${CMAKE_CURRENT_SOURCE_DIR}/cxx03/cxx03Variable.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/cxx03/cxx03Variable.tcc
)

set_property(TARGET adios2_cxx03 PROPERTY CXX_STANDARD 99)

target_include_directories(adios2_cxx03
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

install(
  FILES adios2_cxx03.h 
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cxx03/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cxx03
  FILES_MATCHING 
  PATTERN "*.h" 
)
