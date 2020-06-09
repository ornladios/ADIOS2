#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# This file contains the option and dependency logic.  The configuration
# options are designed to be tertiary: ON, OFF, or AUTO.  If AUTO, we try to
# determine if dependencies are available and enable the option if we find
# them, otherwise we disable it.  If explicitly ON then a failure to find
# dependencies is an error,

# Helper function to extract a common prefix from two strings
function(string_get_prefix in0 in1 outVar)
  string(LENGTH "${in0}" len0)
  string(LENGTH "${in1}" len1)
  set(lenMax ${len0})
  if(len1 LESS len0)
    set(lenMax ${len1})
  endif()
  set(lenPfx 0)
  if(lenMax GREATER 0)
    foreach(len RANGE 1 ${lenMax})
      string(SUBSTRING "${in0}" 0 ${len} sub0)
      string(SUBSTRING "${in1}" 0 ${len} sub1)
      if(NOT (sub0 STREQUAL sub1))
        break()
      endif()
      set(lenPfx ${len})
    endforeach()
  endif()
  string(SUBSTRING "${in0}" 0 ${lenPfx} outTmp)
  set(${outVar} "${outTmp}" PARENT_SCOPE)
endfunction()

# Helper function to strip a common prefix off an input string
function(string_strip_prefix pfx in outVar)
  string(LENGTH "${pfx}" lenPfx)
  string(LENGTH "${in}" lenIn)
  if(lenPfx GREATER lenIn)
    set(${outVar} "" PARENT_SCOPE)
    return()
  endif()
  string(SUBSTRING "${in}" 0 ${lenPfx} inPfx)
  if(NOT pfx STREQUAL inPfx)
    set(${outVar} "" PARENT_SCOPE)
    return()
  endif()
  string(SUBSTRING "${in}" ${lenPfx} -1 outTmp)
  set(${outVar} "${outTmp}" PARENT_SCOPE)
endfunction()

# Extract the common prefix from a collection of variables
function(lists_get_prefix listVars outVar)
  foreach(l IN LISTS listVars)
    foreach(d IN LISTS ${l})
      if(NOT prefix)
        set(prefix "${d}")
        continue()
      endif()
      string_get_prefix("${prefix}" "${d}" prefix)
      if(NOT prefix)
        set(${outVar} "" PARENT_SCOPE)
        return()
      endif()
    endforeach()
  endforeach()
  set(${outVar} "${prefix}" PARENT_SCOPE)
endfunction()

# Blosc
if(ADIOS2_USE_Blosc STREQUAL AUTO)
  find_package(Blosc 1.7)
elseif(ADIOS2_USE_Blosc)
  find_package(Blosc 1.7 REQUIRED)
endif()
if(BLOSC_FOUND)
  set(ADIOS2_HAVE_Blosc TRUE)
endif()

# BZip2
if(ADIOS2_USE_BZip2 STREQUAL AUTO)
  find_package(BZip2)
elseif(ADIOS2_USE_BZip2)
  find_package(BZip2 REQUIRED)
endif()
if(BZIP2_FOUND)
  set(ADIOS2_HAVE_BZip2 TRUE)
endif()

# ZFP
if(ADIOS2_USE_ZFP STREQUAL AUTO)
  find_package(ZFP 0.5.1 CONFIG)
elseif(ADIOS2_USE_ZFP)
  find_package(ZFP 0.5.1 REQUIRED CONFIG)
endif()
if(ZFP_FOUND)
  set(ADIOS2_HAVE_ZFP TRUE)
endif()

# SZ
if(ADIOS2_USE_SZ STREQUAL AUTO)
  find_package(SZ)
elseif(ADIOS2_USE_SZ)
  find_package(SZ REQUIRED)
endif()
if(SZ_FOUND)
  set(ADIOS2_HAVE_SZ TRUE)
endif()

# MGARD
if(ADIOS2_USE_MGARD STREQUAL AUTO)
  find_package(MGARD)
elseif(ADIOS2_USE_MGARD)
  find_package(MGARD REQUIRED)
endif()
if(MGARD_FOUND)
  set(ADIOS2_HAVE_MGARD TRUE)
endif()

# PNG
if(ADIOS2_USE_PNG STREQUAL AUTO)
  find_package(PNG 1.6.0)
elseif(ADIOS2_USE_PNG)
  find_package(PNG 1.6.0 REQUIRED)
endif()
if(PNG_FOUND)
  set(ADIOS2_HAVE_PNG TRUE)
endif()

set(mpi_find_components C)

# Fortran
if(ADIOS2_USE_Fortran STREQUAL AUTO)
  include(CheckLanguage)
  check_language(Fortran)
  if(CMAKE_Fortran_COMPILER)
    enable_language(Fortran)
  endif()
elseif(ADIOS2_USE_Fortran)
  enable_language(Fortran)
endif()
if(CMAKE_Fortran_COMPILER_LOADED)
  set(ADIOS2_HAVE_Fortran TRUE)
  list(APPEND mpi_find_components Fortran)
endif()

# MPI
list(APPEND mpi_find_components OPTIONAL_COMPONENTS CXX)
if(ADIOS2_USE_MPI STREQUAL AUTO)
  find_package(MPI COMPONENTS ${mpi_find_components})
elseif(ADIOS2_USE_MPI)
  find_package(MPI COMPONENTS ${mpi_find_components} REQUIRED)
endif()
if(MPI_FOUND)
  set(ADIOS2_HAVE_MPI TRUE)
  configure_file(
    ${PROJECT_SOURCE_DIR}/scripts/ci/scripts/mpmd-wrapper.sh.cmake
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mpmd-wrapper.sh
    @ONLY
  )
endif()

# ZeroMQ
if(ADIOS2_USE_ZeroMQ STREQUAL AUTO)
    find_package(ZeroMQ 4.1)
elseif(ADIOS2_USE_ZeroMQ)
    find_package(ZeroMQ 4.1 REQUIRED)
endif()
if(ZeroMQ_FOUND)
    set(ADIOS2_HAVE_ZeroMQ TRUE)
endif()

# DataMan
# DataMan currently breaks the PGI compiler
if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "PGI") AND NOT MSVC)
    if(ZeroMQ_FOUND)
        if(ADIOS2_USE_DataMan STREQUAL AUTO)
            set(ADIOS2_HAVE_DataMan TRUE)
        elseif(ADIOS2_USE_DataMan)
            set(ADIOS2_HAVE_DataMan TRUE)
        endif()
    endif()
endif()

# SSC
# SSC currently breaks the PGI compiler
if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "PGI") AND NOT MSVC)
    if(ADIOS2_HAVE_MPI)
        if(ADIOS2_USE_SSC STREQUAL AUTO)
            set(ADIOS2_HAVE_SSC TRUE)
        elseif(ADIOS2_USE_SSC)
            set(ADIOS2_HAVE_SSC TRUE)
        endif()
    endif()
endif()

# Table
if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "PGI") AND NOT MSVC)
    if(ZeroMQ_FOUND)
        if(ADIOS2_USE_Table STREQUAL AUTO)
            set(ADIOS2_HAVE_Table TRUE)
        elseif(ADIOS2_USE_Table)
            set(ADIOS2_HAVE_Table TRUE)
        endif()
    endif()
endif()

# DataSpaces
if(ADIOS2_USE_DataSpaces STREQUAL AUTO)
  find_package(DataSpaces 1.8)
elseif(ADIOS2_USE_DataSpaces)
  find_package(DataSpaces 1.8 REQUIRED)
endif()
if(DATASPACES_FOUND)
  set(ADIOS2_HAVE_DataSpaces TRUE)
endif()

# HDF5
if(ADIOS2_USE_HDF5 STREQUAL AUTO)
  find_package(HDF5 COMPONENTS C)
  if(HDF5_FOUND AND (NOT HDF5_IS_PARALLEL OR ADIOS2_HAVE_MPI))
    set(ADIOS2_HAVE_HDF5 TRUE)
  endif()
elseif(ADIOS2_USE_HDF5)
  find_package(HDF5 REQUIRED COMPONENTS C)
  if(HDF5_IS_PARALLEL AND NOT ADIOS2_HAVE_MPI)
    message(FATAL_ERROR "MPI is disabled but parallel HDF5 is detected.")
  endif()
  set(ADIOS2_HAVE_HDF5 TRUE)
endif()

# IME
if(ADIOS2_USE_IME STREQUAL AUTO)
  find_package(IME)
elseif(ADIOS2_USE_IME)
  find_package(IME REQUIRED)
endif()
if(IME_FOUND)
  set(ADIOS2_HAVE_IME TRUE)
endif()


# Python

# Not supported on PGI
if(CMAKE_CXX_COMPILER_ID STREQUAL PGI)
  if(ADIOS2_USE_Python STREQUAL ON)
    message(FATAL_ERROR "Python bindings are not supported with the PGI compiler")
  elseif(ADIOS2_USE_Python STREQUAL AUTO)
    message(WARNING "Disabling python bindings as they are not supported with the PGI compiler")
    set(ADIOS2_USE_Python OFF)
  endif()
endif()

# Not supported without shared libs
if(NOT SHARED_LIBS_SUPPORTED)
  if(ADIOS2_USE_Python STREQUAL ON)
    message(FATAL_ERROR "Python bindings are not supported without shared library support")
  elseif(ADIOS2_USE_Python STREQUAL AUTO)
    message(WARNING "Disabling python bindings since no shared library support was detected.")
    set(ADIOS2_USE_Python OFF)
  endif()
endif()

if(ADIOS2_USE_Python STREQUAL AUTO)
  find_package(Python COMPONENTS Interpreter Development NumPy)
  if(Python_FOUND AND ADIOS2_HAVE_MPI)
    find_package(PythonModule COMPONENTS mpi4py mpi4py/mpi4py.h)
  endif()
elseif(ADIOS2_USE_Python)
  find_package(Python REQUIRED COMPONENTS Interpreter Development NumPy)
  if(ADIOS2_HAVE_MPI)
    find_package(PythonModule REQUIRED COMPONENTS mpi4py mpi4py/mpi4py.h)
  endif()
endif()
if(Python_FOUND)
  if(ADIOS2_HAVE_MPI)
    if(PythonModule_mpi4py_FOUND)
      set(ADIOS2_HAVE_Python ON)
    endif()
  else()
    set(ADIOS2_HAVE_Python ON)
  endif()
endif()

# Even if no python support, we still want the interpreter for tests
if(NOT Python_Interpreter_FOUND)
  find_package(Python REQUIRED COMPONENTS Interpreter)
endif()

if(Python_Interpreter_FOUND)
  # Setup output directories
  if(Python_Development_FOUND)
    lists_get_prefix("Python_INCLUDE_DIRS;Python_LIBRARIES;Python_SITEARCH" _Python_DEVPREFIX)
  else()
    lists_get_prefix("Python_EXECUTABLE;Python_SITEARCH" _Python_DEVPREFIX)
  endif()
  string_strip_prefix(
    "${_Python_DEVPREFIX}" "${Python_SITEARCH}" CMAKE_INSTALL_PYTHONDIR_DEFAULT
  )
  set(CMAKE_INSTALL_PYTHONDIR "${CMAKE_INSTALL_PYTHONDIR_DEFAULT}"
    CACHE PATH "Install directory for python modules"
  )
  mark_as_advanced(CMAKE_INSTALL_PYTHONDIR)
  set(CMAKE_PYTHON_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}
  )
endif()

# Sst
if(ADIOS2_USE_SST AND NOT MSVC)
  set(ADIOS2_HAVE_SST TRUE)
  find_package(LIBFABRIC 1.6)
  if(LIBFABRIC_FOUND)
    set(ADIOS2_SST_HAVE_LIBFABRIC TRUE)
    find_package(CrayDRC)
    if(CrayDRC_FOUND)
      set(ADIOS2_SST_HAVE_CRAY_DRC TRUE)
    endif()
  endif()
  find_package(NVStream)
  if(NVStream_FOUND)
    find_package(Boost OPTIONAL_COMPONENTS thread log filesystem system)
    set(ADIOS2_SST_HAVE_NVStream TRUE)
  endif()
endif()

#SysV IPC
if(UNIX)
  include(CheckSymbolExists)
  CHECK_SYMBOL_EXISTS(shmget "sys/ipc.h;sys/shm.h" HAVE_shmget)
  if(HAVE_shmget)
    set(ADIOS2_HAVE_SysVShMem ON)
  else()
    set(ADIOS2_HAVE_SysVShMem OFF)
  endif()
else()
  set(ADIOS2_HAVE_SysVShMem OFF)
endif()

#Profiling
if(ADIOS2_USE_Profiling STREQUAL AUTO)
  if(BUILD_SHARED_LIBS)
    set(ADIOS2_HAVE_Profiling ON)
  else()
    set(ADIOS2_HAVE_Profiling OFF)
  endif()
elseif(ADIOS2_USE_Profiling)
  set(ADIOS2_HAVE_Profiling ON)
endif()

if(ADIOS2_USE_Endian_Reverse STREQUAL ON)
  set(ADIOS2_HAVE_Endian_Reverse TRUE)
endif()

# Multithreading
find_package(Threads REQUIRED)

# Floating point detection
include(CheckTypeRepresentation)

#check_float_type_representation(float FLOAT_TYPE_C)
#check_float_type_representation(double DOUBLE_TYPE_C)
#check_float_type_representation("long double" LONG_DOUBLE_TYPE_C)

if(ADIOS2_USE_Fortran)
  #check_float_type_representation(real REAL_TYPE_Fortran LANGUAGE Fortran)
  #check_float_type_representation("real(kind=4)" REAL4_TYPE_Fortran LANGUAGE Fortran)
  #check_float_type_representation("real(kind=8)" REAL8_TYPE_Fortran LANGUAGE Fortran)
  #check_float_type_representation("real(kind=16)" REAL16_TYPE_Fortran LANGUAGE Fortran)

  include(CheckFortranCompilerFlag)
  check_fortran_compiler_flag("-fallow-argument-mismatch" ADIOS2_USE_Fortran_flag_argument_mismatch)
endif()
