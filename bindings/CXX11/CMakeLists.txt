#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

target_sources(adios2 PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/cxx11/ADIOS.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/cxx11/Attribute.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/cxx11/Engine.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/cxx11/Engine.tcc
	${CMAKE_CURRENT_SOURCE_DIR}/cxx11/IO.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/cxx11/IO.tcc
	${CMAKE_CURRENT_SOURCE_DIR}/cxx11/Operator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/cxx11/Variable.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/cxx11/fstream/ADIOS2fstream.cpp
)

target_include_directories(adios2
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

install(
  FILES adios2.h 
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  FILES cxx11/fstream/ADIOS2fstream.h 
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cxx11/fstream
)

install(
  FILES cxx11/ADIOS.h 
        cxx11/ADIOS.inl
        cxx11/IO.h
        cxx11/Variable.h 
        cxx11/Attribute.h
        cxx11/Engine.h
        cxx11/Operator.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cxx11
)
