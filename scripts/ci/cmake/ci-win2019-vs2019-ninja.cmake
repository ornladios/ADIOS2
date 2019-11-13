# Client maintainer: chuck.atkins@kitware.com

set(ENV{CC}  cl)
set(ENV{CXX} cl)
set(ENV{CFLAGS} /WX)
set(ENV{CXXFLAGS} /WX)

set(dashboard_cache "
ADIOS2_USE_Fortran:BOOL=OFF
ADIOS2_USE_MPI:BOOL=OFF
")

set(CTEST_CMAKE_GENERATOR "Ninja")
#set(CTEST_CMAKE_GENERATOR "Visual Studio 16 2019")
#set(CTEST_CMAKE_GENERATOR_PLATFORM "x64") 
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
