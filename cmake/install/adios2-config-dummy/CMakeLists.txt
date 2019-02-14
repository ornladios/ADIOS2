cmake_minimum_required(VERSION 3.0)

project(adios2-config-dummy C CXX)

find_package(adios2 REQUIRED)

if(BUILD_SHARED_LIBS)
  add_library(without_C foo.c)
  add_library(with_C foo.c)
  add_library(without_CXX foo.cxx)
  add_library(with_CXX foo.cxx)
else()
  add_executable(without_C foo.c main.c)
  add_executable(with_C foo.c main.c)
  add_executable(without_CXX foo.cxx main.cxx)
  add_executable(with_CXX foo.cxx main.cxx)
endif()

target_compile_definitions(with_C PRIVATE WITH_ADIOS2)
target_compile_definitions(with_CXX PRIVATE WITH_ADIOS2)
target_link_libraries(with_C adios2::adios2)
target_link_libraries(with_CXX adios2::adios2)

if(ADIOS2_HAVE_Fortran)
  enable_language(Fortran)
  if(BUILD_SHARED_LIBS)
    add_library(without_Fortran foo.F90)
    add_library(with_Fortran foo.F90)
  else()
    add_executable(without_Fortran foo.F90 main.f90)
    add_executable(with_Fortran foo.F90 main.f90)
  endif()
  target_compile_definitions(with_Fortran PRIVATE WITH_ADIOS2)
  target_link_libraries(with_Fortran adios2::adios2_f)
  set_target_properties(without_Fortran with_Fortran
    PROPERTIES
      LINKER_LANGUAGE Fortran
  )
endif()
