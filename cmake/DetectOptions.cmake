#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# We'll need to install the private find modules to ensure our import libs
# are properly resovled
set(adios2_find_modules)

# This file contains the option and dependency logic.  The configuration
# options are designed to be tertiary: ON, OFF, or AUTO.  If AUTO, we try to
# determine if dependencies are available and enable the option if we find
# them, otherwise we disable it.  If explicitly ON then a failure to find
# dependencies is an error,

# BZip2
if(ADIOS2_USE_BZip2 STREQUAL AUTO)
  find_package(BZip2)
  if(BZIP2_FOUND)
    set(ADIOS2_HAVE_BZip2 TRUE)
  endif()
elseif(ADIOS2_USE_BZip2)
  set(ADIOS2_HAVE_BZip2 TRUE)
endif()

# ZFP
if(ADIOS2_USE_ZFP STREQUAL AUTO)
  find_package(ZFP)
  if(ZFP_FOUND)
    set(ADIOS2_HAVE_ZFP TRUE)
  endif()
elseif(ADIOS2_USE_ZFP)
  set(ADIOS2_HAVE_ZFP TRUE)
endif()

# MPI
if(ADIOS2_USE_MPI STREQUAL AUTO)
  find_package(MPI COMPONENTS C)
  if(MPI_FOUND)
    set(ADIOS2_HAVE_MPI TRUE)
  endif()
elseif(ADIOS2_USE_MPI)
  set(ADIOS2_HAVE_MPI TRUE)
endif()

# C
if(ADIOS2_USE_C)
  set(ADIOS2_HAVE_C TRUE)
endif()

# Fortran
if(ADIOS2_USE_Fortran)
  set(ADIOS2_HAVE_Fortran TRUE)
  enable_language(Fortran)
endif()

# DataMan
if(ADIOS2_USE_DataMan STREQUAL AUTO)
  if(SHARED_LIBS_SUPPORTED AND NOT MSVC)
    set(ADIOS2_HAVE_DataMan TRUE)
  endif()
elseif(ADIOS2_USE_DataMan)
  set(ADIOS2_HAVE_DataMan TRUE)
endif()

# ZeroMQ
if(ADIOS2_USE_ZeroMQ STREQUAL AUTO)
  find_package(ZeroMQ)
  if(ZeroMQ_FOUND)
    set(ADIOS2_HAVE_ZeroMQ TRUE)
  endif()
elseif(ADIOS2_USE_ZeroMQ)
  set(ADIOS2_HAVE_ZeroMQ TRUE)
endif()

# HDF5
if(ADIOS2_USE_HDF5 STREQUAL AUTO)
  find_package(HDF5 COMPONENTS C)
  if(HDF5_FOUND AND
     ((ADIOS2_HAVE_MPI AND HDF5_IS_PARALLEL) OR
      NOT (ADIOS2_HAVE_MPI OR HDF5_IS_PARALLEL)))
    set(ADIOS2_HAVE_HDF5 TRUE)
  endif()
elseif(ADIOS2_USE_HDF5)
  set(ADIOS2_HAVE_HDF5 TRUE)
endif()

# ADIOS1
if(ADIOS2_USE_ADIOS1 STREQUAL AUTO)
  if(NOT ADIOS2_HAVE_MPI)
    set(adios1_args COMPONENTS sequential)
  endif()
  find_package(ADIOS1 1.12.0 ${adios1_args})
  unset(adios1_args)
  if(ADIOS1_FOUND)
    set(ADIOS2_HAVE_ADIOS1 TRUE)
  endif()
elseif(ADIOS2_USE_ADIOS1)
  set(ADIOS2_HAVE_ADIOS1 TRUE)
endif()

# Python
# Use the FindPythonLibsNew from pybind11
list(INSERT CMAKE_MODULE_PATH 0
  "${ADIOS2_SOURCE_DIR}/thirdparty/pybind11/pybind11/tools"
)
if(ADIOS2_USE_Python STREQUAL AUTO)
  if(SHARED_LIBS_SUPPORTED AND ADIOS2_ENABLE_PIC)
    set(Python_ADDITIONAL_VERSIONS 3 2.7)
    find_package(PythonInterp)
    find_package(PythonLibsNew)
    if(PYTHONLIBS_FOUND)
      if(ADIOS2_HAVE_MPI)
        find_package(PythonModule COMPONENTS mpi4py mpi4py/mpi4py.h)
        if(PythonModule_mpi4py_FOUND)
          set(ADIOS2_HAVE_Python TRUE)
        endif()
      else()
        set(ADIOS2_HAVE_Python TRUE)
      endif()
    endif()
  endif()
elseif(ADIOS2_USE_Python)
  set(ADIOS2_HAVE_Python TRUE)
endif()

#SysV IPC
if(ADIOS2_USE_SysVShMem STREQUAL AUTO)
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
elseif(ADIOS2_USE_SysVShMem)
  set(ADIOS2_HAVE_SysVShMem TRUE)
endif()
