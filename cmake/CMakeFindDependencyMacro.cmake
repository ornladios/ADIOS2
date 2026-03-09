# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0


# This module is already included in new versions of CMake
if(CMAKE_VERSION VERSION_LESS 3.9)
  include(${CMAKE_CURRENT_LIST_DIR}/upstream/CMakeFindDependencyMacro.cmake)
else()
  include(${CMAKE_ROOT}/Modules/CMakeFindDependencyMacro.cmake)
endif()
