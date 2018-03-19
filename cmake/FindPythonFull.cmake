#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# PythonFull
# -----------
#
# Try to find various pieces needed for a full python environment including
# python libs, interpreter, and modules
#
# This module defines the following variables:
#
#   PythonFull_FOUND
#   PYTHONINTERP_FOUND
#   PYTHON_EXECUTABLE
#   PYTHONLIBS_FOUND
#   PYTHON_INCLUDE_DIRS
#   PYTHON_LIBRARIES
#   PythonModule_${module_NAME}_FOUND
#   
# This is intented to be called by specifying components for libraries,
# interpreter, and modules.  So, for example, if you needed libraries, the
# interpreter, mpi4py, and numpy then you would call:
#
#   find_package(PythonFull COMPONENTS Interp Libs mpi4py numpy)
#

include(CMakeFindDependencyMacro)
set(_req_vars)
foreach(comp IN LISTS PythonFull_FIND_COMPONENTS)
  if(comp STREQUAL Interp)
    find_package(PythonInterp)
    set(PythonFull_${comp}_FOUND ${PYTHONINTERP_FOUND})
    list(APPEND _req_vars PYTHON_EXECUTABLE)
  elseif(comp STREQUAL Libs)
    if(PYTHONINTERP_FOUND AND Python_ADDITIONAL_VERSIONS)
      set(_Python_ADDITIONAL_VERSIONS "${Python_ADDITIONAL_VERSIONS}")
      unset(Python_ADDITIONAL_VERSIONS)
      unset(Python_ADDITIONAL_VERSIONS CACHE)
      find_package(PythonLibs)
      set(Python_ADDITIONAL_VERSIONS "${_Python_ADDITIONAL_VERSIONS}"
        CACHE STRING "Python versions to search for"
      )
      unset(_Python_ADDITIONAL_VERSIONS)
    else()
      find_package(PythonLibs)
    endif()
    set(PythonFull_${comp}_FOUND ${PYTHONLIBS_FOUND})
    list(APPEND _req_vars PYTHON_LIBRARIES)
  else()
    # Parse off any specified headers and libs for a given module
    set(_comp ${comp})
    list(GET _comp 0 _mod)
    string(REGEX REPLACE "${_mod}\\\;[^;]*" "${_mod}"
      PythonFull_FIND_COMPONENTS "${PythonFull_FIND_COMPONENTS}"
    )
    set(PythonFull_FIND_REQUIRED_${_mod} TRUE)
    find_package(PythonModule COMPONENTS ${_comp})
    set(PythonFull_${_mod}_FOUND ${PythonModule_${_mod}_FOUND})
  endif()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PythonFull
  REQUIRED_VARS ${_req_vars}
  HANDLE_COMPONENTS
)
