#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#


include(FortranCInterface)
FortranCInterface_HEADER(FC.h MACRO_NAMESPACE "FC_")
FortranCInterface_VERIFY(CXX QUIET)

set(F2C
     f2c/adios2_f2c_adios.cpp  
     f2c/adios2_f2c_io.cpp 
     f2c/adios2_f2c_variable.cpp
     f2c/adios2_f2c_engine.cpp
)

set(MODULES 
     adios2_mod.f90
     modules/adios2_functions_mod.f90 
     modules/adios2_parameters_mod.f90 
     modules/adios2_adios_mod.f90 
     modules/adios2_io_mod.f90 
     modules/adios2_variable_mod.f90
     modules/adios2_engine_mod.f90 
     modules/adios2_engine_write_mod.f90 
     modules/adios2_engine_iwrite_mod.f90
     modules/adios2_engine_read_mod.f90 
     modules/adios2_engine_iread_mod.f90
)

add_library(adios2_f ${MODULES} ${F2C})
target_include_directories(adios2_f PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} 
                                           ${CMAKE_CURRENT_BINARY_DIR}
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


install(
  TARGETS adios2_f EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
