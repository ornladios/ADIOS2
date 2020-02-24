#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
include(FortranCInterface)
FortranCInterface_HEADER(FC.h MACRO_NAMESPACE "FC_")
FortranCInterface_VERIFY(CXX QUIET)

add_library(adios2_f
  f2c/adios2_f2c_adios.cpp
  f2c/adios2_f2c_attribute.cpp
  f2c/adios2_f2c_io.cpp
  f2c/adios2_f2c_variable.cpp
  f2c/adios2_f2c_engine.cpp
  f2c/adios2_f2c_operator.cpp

  modules/adios2_mod.f90
  modules/adios2_functions_mod.f90
  modules/adios2_functions_allocate_mod.f90
  modules/adios2_parameters_mod.f90
  modules/adios2_adios_mod.f90
  modules/adios2_adios_init_mod.F90
  modules/adios2_attribute_mod.f90
  modules/adios2_attribute_data_mod.f90
  modules/adios2_io_mod.f90
  modules/adios2_io_open_mod.F90
  modules/adios2_io_define_variable_mod.f90
  modules/adios2_io_define_attribute_mod.f90
  modules/adios2_engine_mod.f90
  modules/adios2_engine_begin_step_mod.f90
  modules/adios2_engine_put_mod.f90
  modules/adios2_engine_get_mod.f90
  modules/adios2_variable_mod.f90
  modules/adios2_variable_min_mod.f90
  modules/adios2_variable_max_mod.f90
  modules/adios2_operator_mod.f90
)

target_include_directories(adios2_f
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/adios2/fortran>
)

target_link_libraries(adios2_f PRIVATE adios2)


if(ADIOS2_HAVE_MPI)
  target_sources(adios2_f PRIVATE
    f2c/adios2_f2c_adios_mpi.cpp
    f2c/adios2_f2c_io_mpi.cpp
  )
  target_compile_definitions(adios2_f PUBLIC ADIOS2_HAVE_MPI_F)
  target_link_libraries(adios2_f PRIVATE MPI::MPI_Fortran MPI::MPI_C)
endif()


install(
  TARGETS adios2_f EXPORT adios2Exports
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(
  DIRECTORY ${CMAKE_Fortran_MODULE_DIRECTORY}/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2/fortran
  FILES_MATCHING 
    PATTERN "adios2*.mod" 
    PATTERN "CMakeFiles" EXCLUDE
)
