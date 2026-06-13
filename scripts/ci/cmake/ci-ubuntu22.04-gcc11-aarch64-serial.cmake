# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

include(ProcessorCount)
ProcessorCount(NCPUS)
math(EXPR N2CPUS "${NCPUS}*2")

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)

set(dashboard_cache "
BUILD_TESTING:BOOL=ON
ADIOS2_BUILD_EXAMPLES:BOOL=ON

ADIOS2_USE_BZip2:BOOL=ON
ADIOS2_USE_DataMan:BOOL=ON
ADIOS2_USE_Fortran:BOOL=ON
ADIOS2_USE_HDF5:BOOL=OFF
ADIOS2_USE_MPI:BOOL=OFF
ADIOS2_USE_Python:BOOL=ON
ADIOS2_USE_ZeroMQ:BOOL=ON

CMAKE_C_COMPILER_LAUNCHER=ccache
CMAKE_CXX_COMPILER_LAUNCHER=ccache
CMAKE_C_FLAGS:STRING=-Wall -Wno-psabi
CMAKE_CXX_FLAGS:STRING=-Wall -Wno-psabi
CMAKE_Fortran_FLAGS:STRING=-Wall
")

#set(CTEST_TEST_ARGS EXCLUDE "KillReader|KillWriter|PreciousTimestep")

set(CTEST_CMAKE_GENERATOR "Ninja")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
