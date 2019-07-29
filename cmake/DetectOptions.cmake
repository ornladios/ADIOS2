#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# This file contains the option and dependency logic.  The configuration
# options are designed to be tertiary: ON, OFF, or AUTO.  If AUTO, we try to
# determine if dependencies are available and enable the option if we find
# them, otherwise we disable it.  If explicitly ON then a failure to find
# dependencies is an error,

# Blosc
if(ADIOS2_USE_Blosc STREQUAL AUTO)
  find_package(Blosc)
elseif(ADIOS2_USE_Blosc)
  find_package(Blosc REQUIRED)
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
  find_package(ZFP)
elseif(ADIOS2_USE_ZFP)
  find_package(ZFP REQUIRED)
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
    if(ZeroMQ_FOUND)
        if(ADIOS2_USE_SSC STREQUAL AUTO)
            set(ADIOS2_HAVE_SSC TRUE)
        elseif(ADIOS2_USE_SSC)
            set(ADIOS2_HAVE_SSC TRUE)
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
  if(HDF5_FOUND AND
     ((ADIOS2_HAVE_MPI AND HDF5_IS_PARALLEL) OR
      (NOT ADIOS2_HAVE_MPI AND NOT HDF5_IS_PARALLEL)))
    set(ADIOS2_HAVE_HDF5 TRUE)
  endif()
elseif(ADIOS2_USE_HDF5)
  find_package(HDF5 REQUIRED COMPONENTS C)
  if(ADIOS2_HAVE_MPI)
    if(NOT HDF5_IS_PARALLEL)
      message(FATAL_ERROR "MPI is enabled but serial HDF5 is detected.")
    endif()
  else()
    if(HDF5_IS_PARALLEL)
      message(FATAL_ERROR "MPI is disabled but parallel HDF5 is detected.")
    endif()
  endif()
  set(ADIOS2_HAVE_HDF5 TRUE)
endif()

# Python
if(CMAKE_CXX_COMPILER_ID STREQUAL PGI)
  if(ADIOS2_USE_Python STREQUAL ON)
    message(FATAL_ERROR "Python bindings are not supported with the PGI compiler")
  elseif(ADIOS2_USE_Python STREQUAL AUTO)
    message(WARNING "Disabling python bindings as they are not supported with the PGI compiler")
    set(ADIOS2_USE_Python OFF)
  endif()
endif()
if(ADIOS2_USE_Python)
  if(NOT (ADIOS2_USE_Python STREQUAL AUTO))
    set(python_find_args REQUIRED)
  endif()
  if(BUILD_SHARED_LIBS)
    set(Python_ADDITIONAL_VERSIONS "3;2.7"
      CACHE STRING "Python versions to search for"
    )
    mark_as_advanced(Python_ADDITIONAL_VERSIONS)
    list(APPEND python_find_args COMPONENTS Interp Libs numpy)
    if(ADIOS2_HAVE_MPI)
      list(APPEND python_find_args "mpi4py\\\;mpi4py/mpi4py.h")
    endif()
    find_package(PythonFull ${python_find_args})
  endif()
endif()
if(PythonFull_FOUND)
  set(ADIOS2_HAVE_Python ON)
endif()

# Sst
if(ADIOS2_USE_SST AND NOT MSVC)
  set(ADIOS2_HAVE_SST TRUE)
  find_package(LIBFABRIC 1.6)
  if(LIBFABRIC_FOUND)
    set(ADIOS2_SST_HAVE_LIBFABRIC TRUE)
    find_package(CrayDRC)
    if(CRAY_DRC_FOUND)
      set(ADIOS2_SST_HAVE_CRAY_DRC TRUE)
    endif()
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
