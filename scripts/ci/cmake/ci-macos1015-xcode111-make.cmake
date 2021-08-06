# Client maintainer: chuck.atkins@kitware.com
set(ENV{CC}  clang)
set(ENV{CXX} clang++)
set(ENV{FC}  gfortran)

set(dashboard_cache "
ADIOS2_USE_Fortran:BOOL=ON
ADIOS2_USE_MPI:BOOL=OFF
ADISO2_USE_Python:BOOL=ON

CMAKE_C_FLAGS:STRING=-Wall
CMAKE_CXX_FLAGS:STRING=-Wall
CMAKE_Fortran_FLAGS:STRING=-Wall
")

set(ENV{MACOSX_DEPLOYMENT_TARGET} "10.15")
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
