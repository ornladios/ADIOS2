cmake_minimum_required(VERSION 3.0)

project(adios2-config-dummy C CXX)

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

find_package(adios2 REQUIRED)

add_executable(without_C foo.c main.c)
add_executable(with_C foo.c main.c)
add_executable(without_CXX foo.cxx main.cxx)
add_executable(with_CXX foo.cxx main.cxx)

target_compile_definitions(with_C PRIVATE WITH_ADIOS2)
target_compile_definitions(with_CXX PRIVATE WITH_ADIOS2)
target_link_libraries(with_C adios2::adios2)
target_link_libraries(with_CXX adios2::adios2)

if(ADIOS2_HAVE_Fortran)
  enable_language(Fortran)
  add_executable(without_Fortran foo.F90 main.f90)
  add_executable(with_Fortran foo.F90 main.f90)
  target_compile_definitions(with_Fortran PRIVATE WITH_ADIOS2)
  target_link_libraries(with_Fortran adios2::adios2_f)

  set_target_properties(without_Fortran with_Fortran
    PROPERTIES
      LINKER_LANGUAGE Fortran
  )
endif()

if(ADIOS2_HAVE_MPI)
  find_package(MPI REQUIRED)
  target_link_libraries(without_C MPI::MPI_C)
  target_link_libraries(without_CXX MPI::MPI_C)
  if(ADIOS2_HAVE_Fortran)
    target_link_libraries(without_Fortran MPI::MPI_Fortran)
  endif()
endif()
