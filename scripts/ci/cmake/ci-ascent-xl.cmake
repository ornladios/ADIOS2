# Client maintainer: vicente.bolea@kitware.com

set(ENV{CC}  xlc)
set(ENV{CXX} xlc++)
set(ENV{FC}  xlf)

set(dashboard_cache "
BUILD_TESTING:BOOL=ON
ADIOS2_BUILD_EXAMPLES:BOOL=ON

ADIOS2_USE_BZip2:BOOL=OFF
ADIOS2_USE_DataMan:BOOL=ON
ADIOS2_USE_Fortran:BOOL=OFF
ADIOS2_USE_MPI:BOOL=OFF
ADIOS2_USE_PNG:BOOL=OFF
ADIOS2_USE_Python:BOOL=OFF
ADIOS2_USE_SST:BOOL=ON
ADIOS2_USE_ZeroMQ:STRING=OFF
ADIOS2_USE_ZFP:BOOL=OFF
ADIOS2_USE_SZ:BOOL=OFF
ADIOS2_USE_Blosc2:BOOL=OFF

CMAKE_C_COMPILER_LAUNCHER=ccache
CMAKE_CXX_COMPILER_LAUNCHER=ccache
CMAKE_DISABLE_FIND_PACKAGE_BISON=ON
CMAKE_DISABLE_FIND_PACKAGE_FLEX=ON
")

set(NCPUS 4)
set(CTEST_CMAKE_GENERATOR "Ninja")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-ascent-common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
