# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# This module is already included in new versions of CMake
if(CMAKE_VERSION VERSION_LESS 3.18.0)
  include(${CMAKE_CURRENT_LIST_DIR}/upstream/FindPython.cmake)
else()
  include(${CMAKE_ROOT}/Modules/FindPython.cmake)
endif()

# Backwards compatibility with deprecated FindPythonInterp.cmake
if(Python_Interpreter_FOUND)
  set(PYTHON_EXECUTABLE "${Python_EXECUTABLE}" CACHE INTERNAL
    "Helper for deprecated FindPythonInterp" FORCE
  )
endif()
