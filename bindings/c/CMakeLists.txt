#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(adios2_c_targets)

add_library(adios2_c adios2_c.cpp)
target_link_libraries(adios2_c PRIVATE adios2)

if(ADIOS2_HAVE_MPI)
  target_compile_definitions(adios2_c PUBLIC ADIOS2_HAVE_MPI_C)
  target_include_directories(adios2_c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${MPI_C_INCLUDE_PATH})
  target_link_libraries(adios2_c PRIVATE ${MPI_C_LIBRARIES})
else()
  target_include_directories(adios2_c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()

list(APPEND adios2_c_targets adios2_c)

install(
  TARGETS ${adios2_c_targets} EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  FILES adios2_c.h 
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  FILES adios2/adios2_c_enums.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2
)
