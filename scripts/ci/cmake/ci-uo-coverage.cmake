# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Client maintainer: vicente.bolea@kitware.com

set(ENV{CC}  gcc)
set(ENV{CXX} g++)

find_program(CTEST_COVERAGE_COMMAND NAMES gcov)

set(dashboard_cache "
BUILD_TESTING:BOOL=ON
ADIOS2_BUILD_EXAMPLES:BOOL=ON

ADIOS2_USE_Fortran:STRING=OFF
ADIOS2_USE_HDF5:STRING=ON
ADIOS2_USE_MPI:STRING=OFF
ADIOS2_USE_Python:STRING=OFF
ADIOS2_USE_ZFP:STRING=ON

CMAKE_BUILD_TYPE:STRING=Debug
CMAKE_C_FLAGS:STRING=--coverage
CMAKE_CXX_FLAGS:STRING=--coverage
CMAKE_EXE_LINKER_FLAGS:STRING=--coverage
CMAKE_MODULE_LINKER_FLAGS:STRING=--coverage
CMAKE_SHARED_LINKER_FLAGS:STRING=--coverage
")

set(dashboard_track "Analysis")
set(CTEST_CMAKE_GENERATOR "Ninja")
set(CTEST_BUILD_CONFIGURATION Debug)

set(ADIOS_TEST_REPEAT 0)
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
