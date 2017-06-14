#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# This file contains the option and dependency logic.  The configuration
# options are designed to be tertiary: ON, OFF, or AUTO.  If AUTO, we try to
# determine if dependencies are available and enable the option if we find
# them, otherwise we disable it.  If explicitly ON then a failure to find
# dependencies is an error,

# BZip2
if(ADIOS_USE_BZip2 STREQUAL AUTO)
  find_package(BZip2)
  if(BZIP2_FOUND)
    set(ADIOS_HAVE_BZip2 TRUE)
  endif()
elseif(ADIOS_USE_BZip2)
  set(ADIOS_HAVE_BZip2 TRUE)
endif()

# ZFP
if(ADIOS_USE_ZFP STREQUAL AUTO)
  find_package(ZFP)
  if(ZFP_FOUND)
    set(ADIOS_HAVE_ZFP TRUE)
  endif()
elseif(ADIOS_USE_ZFP)
  set(ADIOS_HAVE_ZFP TRUE)
endif()

# MPI
if(ADIOS_USE_MPI STREQUAL AUTO)
  find_package(MPI COMPONENTS C)
  if(MPI_FOUND)
    set(ADIOS_HAVE_MPI TRUE)
  endif()
elseif(ADIOS_USE_MPI)
  set(ADIOS_HAVE_MPI TRUE)
endif()

# DataMan
if(ADIOS_USE_DataMan STREQUAL AUTO)
  if(SHARED_LIBS_SUPPORTED AND NOT MSVC)
    set(ADIOS_HAVE_DataMan TRUE)
  endif()
elseif(ADIOS_USE_DataMan)
  set(ADIOS_HAVE_DataMan TRUE)
endif()

# ZeroMQ
if(ADIOS_USE_ZeroMQ STREQUAL AUTO)
  find_package(ZeroMQ)
  if(ZeroMQ_FOUND)
    set(ADIOS_HAVE_ZeroMQ TRUE)
  endif()
elseif(ADIOS_USE_ZeroMQ)
  set(ADIOS_HAVE_ZeroMQ TRUE)
endif()

# HDF5
if(ADIOS_USE_HDF5 STREQUAL AUTO)
  find_package(HDF5 COMPONENTS C)
  if(HDF5_FOUND AND
     ((ADIOS_HAVE_MPI AND HDF5_IS_PARALLEL) OR
      NOT (ADIOS_HAVE_MPI OR HDF5_IS_PARALLEL)))
    set(ADIOS_HAVE_HDF5 TRUE)
  endif()
elseif(ADIOS_USE_HDF5)
  set(ADIOS_HAVE_HDF5 TRUE)
endif()

# ADIOS1
if(ADIOS_USE_ADIOS1 STREQUAL AUTO)
  if(NOT ADIOS_HAVE_MPI)
    set(adios1_args COMPONENTS sequential)
  endif()
  find_package(ADIOS1 1.12.0 ${adios1_args})
  unset(adios1_args)
  if(ADIOS1_FOUND)
    set(ADIOS_HAVE_ADIOS1 TRUE)
  endif()
elseif(ADIOS_USE_ADIOS1)
  set(ADIOS_HAVE_ADIOS1 TRUE)
endif()

# Python
if(ADIOS_USE_Python STREQUAL AUTO)
  if(BUILD_SHARED_LIBS)
    find_package(PythonLibs)
    if(PYTHONLIBS_FOUND)
      if(ADIOS_HAVE_MPI)
        find_package(PythonModule COMPONENTS mpi4py mpi4py/mpi4py.h)
        if(PythonModule_mpi4py_FOUND)
          set(ADIOS_HAVE_Python TRUE)
        endif()
      else()
        set(ADIOS_HAVE_Python TRUE)
      endif()
    endif()
  endif()
elseif(ADIOS_USE_Python)
  set(ADIOS_HAVE_Python TRUE)
endif()
