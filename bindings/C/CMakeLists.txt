#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#


add_library(adios2_c
  adios2/c/adios2_c_adios.cpp
  adios2/c/adios2_c_io.cpp
  adios2/c/adios2_c_engine.cpp
  adios2/c/adios2_c_variable.cpp
  adios2/c/adios2_c_attribute.cpp
  adios2/c/adios2_c_operator.cpp
)
set_property(TARGET adios2_c PROPERTY EXPORT_NAME c)
target_link_libraries(adios2_c PRIVATE adios2_core)

target_include_directories(adios2_c
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(ADIOS2_HAVE_MPI)
  add_library(adios2_c_mpi
    adios2/c/adios2_c_adios_mpi.cpp
    adios2/c/adios2_c_io_mpi.cpp
    )
  set_property(TARGET adios2_c_mpi PROPERTY EXPORT_NAME c_mpi)
  target_link_libraries(adios2_c_mpi PUBLIC adios2_c PRIVATE adios2_core_mpi PUBLIC MPI::MPI_C)
  target_compile_definitions(adios2_c_mpi INTERFACE ADIOS2_USE_MPI)
  set(maybe_adios2_c_mpi adios2_c_mpi)
else()
  set(maybe_adios2_c_mpi)
endif()

# Set library version information
set_target_properties(
  adios2_c ${maybe_adios2_c_mpi}
  PROPERTIES
  VERSION ${ADIOS2_LIBRARY_VERSION}
  SOVERSION ${ADIOS2_VERSION_MAJOR}
)

install(TARGETS adios2_c ${maybe_adios2_c_mpi} EXPORT adios2Exports
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

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
