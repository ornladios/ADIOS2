# Client maintainer: chuck.atkins@kitware.com
set(ENV{CC}  clang)
set(ENV{CXX} clang++)

set(dashboard_cache "
ADIOS2_USE_Fortran:BOOL=OFF
ADIOS2_USE_MPI:BOOL=OFF
")

set(ENV{MACOSX_DEPLOYMENT_TARGET} "10.13")
set(CTEST_CMAKE_GENERATOR "Xcode")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
