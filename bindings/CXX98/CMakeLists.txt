#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_library(adios2_cxx98
	${CMAKE_CURRENT_SOURCE_DIR}/cxx98/cxx98ADIOS.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/cxx98/cxx98Attribute.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/cxx98/cxx98Attribute.tcc
	${CMAKE_CURRENT_SOURCE_DIR}/cxx98/cxx98Engine.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/cxx98/cxx98IO.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/cxx98/cxx98IO.tcc
	${CMAKE_CURRENT_SOURCE_DIR}/cxx98/cxx98Variable.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/cxx98/cxx98Variable.tcc
)

set_property(TARGET adios2_cxx98 PROPERTY CXX_STANDARD 98)


target_include_directories(adios2_cxx98 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} 
                                               ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(adios2_cxx98 PUBLIC adios2)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(adios2_cxx98 PRIVATE MPI::MPI_C)
endif()

install(
  TARGETS adios2_cxx98 EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  FILES adios2_cxx98.h 
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cxx98/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cxx98
  FILES_MATCHING 
  PATTERN "*.h" 
)
