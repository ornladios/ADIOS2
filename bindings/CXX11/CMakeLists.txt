#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

target_sources(adios2 PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/adios2/cxx11/ADIOS.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/adios2/cxx11/Attribute.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/adios2/cxx11/Engine.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/adios2/cxx11/Engine.tcc
	${CMAKE_CURRENT_SOURCE_DIR}/adios2/cxx11/IO.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/adios2/cxx11/IO.tcc
	${CMAKE_CURRENT_SOURCE_DIR}/adios2/cxx11/Operator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/adios2/cxx11/Query.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/adios2/cxx11/Types.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/adios2/cxx11/Types.tcc
	${CMAKE_CURRENT_SOURCE_DIR}/adios2/cxx11/Variable.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/adios2/cxx11/fstream/ADIOS2fstream.cpp
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
  FILES adios2/cxx11/fstream/ADIOS2fstream.h 
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2/cxx11/fstream
)

install(
  FILES adios2/cxx11/ADIOS.h 
        adios2/cxx11/ADIOS.inl
        adios2/cxx11/IO.h
        adios2/cxx11/Variable.h 
        adios2/cxx11/Attribute.h
        adios2/cxx11/Engine.h
        adios2/cxx11/Operator.h
        adios2/cxx11/Query.h
        adios2/cxx11/Types.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2/cxx11
)
