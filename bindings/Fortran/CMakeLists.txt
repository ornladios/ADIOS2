#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
include(FortranCInterface)
FortranCInterface_HEADER(FC.h MACRO_NAMESPACE "FC_")
FortranCInterface_VERIFY(CXX QUIET)

set(F2C
     ${CMAKE_CURRENT_SOURCE_DIR}/f2c/adios2_f2c_adios.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/f2c/adios2_f2c_attribute.cpp 
     ${CMAKE_CURRENT_SOURCE_DIR}/f2c/adios2_f2c_io.cpp 
     ${CMAKE_CURRENT_SOURCE_DIR}/f2c/adios2_f2c_variable.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/f2c/adios2_f2c_engine.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/f2c/adios2_f2c_operator.cpp
)

set(MODULES 
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_mod.f90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_functions_mod.f90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_functions_allocate_mod.F90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_parameters_mod.F90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_adios_mod.f90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_attribute_mod.f90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_attribute_data_mod.F90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_io_mod.f90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_io_define_variable_mod.f90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_io_define_attribute_mod.F90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_engine_mod.f90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_engine_begin_step_mod.f90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_engine_put_mod.F90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_engine_get_mod.F90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_variable_mod.f90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_variable_min_mod.F90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_variable_max_mod.F90
     ${CMAKE_CURRENT_SOURCE_DIR}/modules/adios2_operator_mod.f90
)

add_library(adios2_f ${MODULES} ${F2C})
target_include_directories(adios2_f
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/adios2/fortran>
)

target_link_libraries(adios2_f PRIVATE adios2)


if(ADIOS2_HAVE_MPI)
   target_sources(adios2_f PRIVATE modules/mpi/adios2_adios_init_mod.f90 
                                   modules/mpi/adios2_io_open_mod.f90
   )
   target_compile_definitions(adios2_f PUBLIC ADIOS2_HAVE_MPI_F)
   target_link_libraries(adios2_f PRIVATE MPI::MPI_Fortran MPI::MPI_C)
else()
   target_sources(adios2_f PRIVATE modules/nompi/adios2_adios_init_mod.f90 
                                   modules/nompi/adios2_io_open_mod.f90
   )
endif()

# feature check: real(kind=16) types are not supported with PGI
function(_Fortran_has_real16 success)
  if(NOT ${success})
    set(scratch_directory
      ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/fortran)
    set(test_file ${scratch_directory}/cmake_real16_test.f90)
    file(WRITE ${test_file}
      "program fortran_real16_hello\n"
      "  real(kind=16) :: x\n"
      "end\n")
    try_compile(${success} ${scratch_directory} ${test_file})
    if(${success})
        message(STATUS "Fortran: real(kind=16) supported")
        set(${success} TRUE PARENT_SCOPE)
    else()
        set(${success} FALSE PARENT_SCOPE)
        message(STATUS "Fortran: real(kind=16) NOT supported")
    endif()
  endif()
endfunction()

set(ADIOS2_HAVE_Fortran_REAL16 FALSE)
_Fortran_has_real16(ADIOS2_HAVE_Fortran_REAL16)
if(Fortran_HAS_REAL16)
  target_compile_definitions(adios2_f PUBLIC ADIOS2_HAVE_Fortran_REAL16)
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
