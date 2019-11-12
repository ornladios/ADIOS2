#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

function(add_install_cmake_test name)
  add_test(NAME Install.CMake.${name} COMMAND "${CMAKE_COMMAND}"
    "-DADIOS2_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
    "-DADIOS2_BINARY_DIR=${CMAKE_BINARY_DIR}"
    "-DCMAKE_GENERATOR=${CMAKE_GENERATOR}"
    "-DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
    "-DCMAKE_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
    "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
    "-DTEST_CASE=${name}"
    "-DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}"
    "-DCMAKE_INSTALL_CMAKEDIR=${CMAKE_INSTALL_CMAKEDIR}"
    "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DBUILD_TYPE=$<CONFIG>"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/run_cmake.cmake"
    )
  set_tests_properties(Install.CMake.${name} PROPERTIES
    DEPENDS Install.Setup
    FIXTURES_REQUIRED Install
    )
endfunction()

function(add_install_make_test name)
  if(ADIOS2_HAVE_MPI)
    set(cc "${MPI_C_COMPILER}")
    set(cxx "${MPI_CXX_COMPILER}")
    set(fc "${MPI_Fortran_COMPILER}")
    set(mpiexec "${MPIEXEC_EXECUTABLE}")
  else()
    set(cc "${CMAKE_C_COMPILER}")
    set(cxx "${CMAKE_CXX_COMPILER}")
    set(fc "${CMAKE_Fortran_COMPILER}")
    set(mpiexec)
  endif()
  add_test(NAME Install.Make.${name} COMMAND "${CMAKE_COMMAND}"
    "-DADIOS2_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
    "-DADIOS2_BINARY_DIR=${CMAKE_BINARY_DIR}"
    "-DADIOS2_HAVE_MPI=${ADIOS2_HAVE_MPI}"
    "-DMAKE_COMMAND=${MAKE_COMMAND}"
    "-DCMAKE_C_COMPILER=${cc}"
    "-DCMAKE_CXX_COMPILER=${cxx}"
    "-DCMAKE_Fortran_COMPILER=${fc}"
    "-DTEST_CASE=${name}"
    "-DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}"
    "-DMPIEXEC_EXECUTABLE=${mpiexec}"
    "-DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/run_make.cmake"
    )
  set_tests_properties(Install.Make.${name} PROPERTIES
    DEPENDS Install.Setup
    FIXTURES_REQUIRED Install
    )
endfunction()

add_test(NAME Install.Setup COMMAND "${CMAKE_COMMAND}"
  "-DADIOS2_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
  "-DADIOS2_BINARY_DIR=${CMAKE_BINARY_DIR}"
  "-DBUILD_TYPE=$<CONFIG>"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/run_install.cmake"
  )
set_tests_properties(Install.Setup PROPERTIES
  FIXTURES_SETUP Install
  )

add_install_cmake_test(C)
add_install_cmake_test(CXX11)
if(ADIOS2_HAVE_Fortran)
  add_install_cmake_test(Fortran)
endif()

if(NOT WIN32)
  find_program(MAKE_COMMAND make)
  if(MAKE_COMMAND)
    add_install_make_test(C)
    add_install_make_test(CXX11)
    if(ADIOS2_HAVE_Fortran)
      add_install_make_test(Fortran)
    endif()
  endif()
endif()
