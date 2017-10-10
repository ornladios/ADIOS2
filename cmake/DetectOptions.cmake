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

set(mpi_find_components C)

# Fortran
if(ADIOS2_USE_Fortran STREQUAL AUTO)
  # Currently auto-detection for language support does not work in CMake.  See
  # documentation for the "enable_language" command
  message(WARN "Auto-detection of Fortran is not currently supported; Disabling")
  #enable_language(Fortran OPTIONAL)
elseif(ADIOS2_USE_Fortran)
  enable_language(Fortran)
endif()
if(CMAKE_Fortran_COMPILER_LOADED)
  set(ADIOS2_HAVE_Fortran TRUE)
  list(APPEND mpi_find_components Fortran)
endif()

# MPI
if(ADIOS2_USE_MPI STREQUAL AUTO)
  find_package(MPI COMPONENTS ${mpi_find_components})
elseif(ADIOS2_USE_MPI)
  find_package(MPI COMPONENTS ${mpi_find_components} REQUIRED)
endif()
if(MPI_FOUND)
  set(ADIOS2_HAVE_MPI TRUE)
endif()

# DataMan
if(SHARED_LIBS_SUPPORTED AND NOT MSVC)
  set(ADIOS2_HAVE_DataMan TRUE)
endif()

# ZeroMQ
if(ADIOS2_USE_ZeroMQ STREQUAL AUTO)
  find_package(ZeroMQ)
elseif(ADIOS2_USE_ZeroMQ)
  find_package(ZeroMQ REQUIRED)
endif()
if(ZeroMQ_FOUND)
  set(ADIOS2_HAVE_ZeroMQ TRUE)
endif()

# HDF5
if(ADIOS2_USE_HDF5 STREQUAL AUTO)
  find_package(HDF5 COMPONENTS C)
elseif(ADIOS2_USE_HDF5)
  find_package(HDF5 REQUIRED COMPONENTS C)
endif()
if(HDF5_FOUND AND
   ((ADIOS2_HAVE_MPI AND HDF5_IS_PARALLEL) OR
    NOT (ADIOS2_HAVE_MPI OR HDF5_IS_PARALLEL)))
  set(ADIOS2_HAVE_HDF5 TRUE)
endif()

# ADIOS1
if(NOT ADIOS2_HAVE_MPI)
  set(adios1_find_args COMPONENTS sequential)
endif()
if(ADIOS2_USE_ADIOS1 STREQUAL AUTO)
  find_package(ADIOS1 1.12.0 ${adios1_find_args})
elseif(ADIOS2_USE_ADIOS1)
  find_package(ADIOS1 1.12.0 REQUIRED ${adios1_find_args})
endif()
if(ADIOS1_FOUND)
  set(ADIOS2_HAVE_ADIOS1 TRUE)
endif()

# Python
# Use the FindPythonLibsNew from pybind11
list(INSERT CMAKE_MODULE_PATH 0
  "${ADIOS2_SOURCE_DIR}/thirdparty/pybind11/pybind11/tools"
)
if(ADIOS2_USE_Python)
  if(NOT (ADIOS2_USE_Python STREQUAL AUTO))
    set(python_find_args REQUIRED)
  endif()
  if(SHARED_LIBS_SUPPORTED AND ADIOS2_ENABLE_PIC)
    set(Python_ADDITIONAL_VERSIONS 3 2.7)
    list(APPEND python_find_args COMPONENTS Interp Libs numpy)
    if(ADIOS2_HAVE_MPI)
      list(APPEND python_find_args "mpi4py\\\;mpi4py/mpi4py.h")
    endif()
    find_package(PythonFull ${python_find_args})
  endif()
endif()
if(PythonFull_FOUND)
  set(ADIOS2_HAVE_Python ON)
  message("PYTHONLIBS_FOUND = ${PYTHONLIBS_FOUND}")
  message("PYTHON_PREFIX = ${PYTHON_PREFIX}")
  message("PYTHON_LIBRARIES = ${PYTHON_LIBRARIES}")
  message("PYTHON_INCLUDE_DIRS = ${PYTHON_INCLUDE_DIRS}")
  message("PYTHON_MODULE_EXTENSION = ${PYTHON_MODULE_EXTENSION}")
  message("PYTHON_MODULE_PREFIX = ${PYTHON_MODULE_PREFIX}")
  message("PYTHON_SITE_PACKAGES = ${PYTHON_SITE_PACKAGES}")
  message("PYTHON_IS_DEBUG = ${PYTHON_IS_DEBUG}")
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

# Multithreading
find_package(Threads REQUIRED)
