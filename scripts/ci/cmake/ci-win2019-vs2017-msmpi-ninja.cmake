# Client maintainer: chuck.atkins@kitware.com

set(ENV{CC}  cl)
set(ENV{CXX} cl)
#set(ENV{CFLAGS} /WX)
#set(ENV{CXXFLAGS} /WX)

include(ProcessorCount)
ProcessorCount(NCPUS)
math(EXPR N2CPUS "${NCPUS}*2")

set(dashboard_cache "
ADIOS2_USE_Fortran:BOOL=OFF
ADIOS2_USE_MPI:STRING=ON

MPIEXEC_MAX_NUMPROCS:STRING=${N2CPUS}
")

set(CTEST_TEST_ARGS PARALLEL_LEVEL 1)
set(CTEST_CMAKE_GENERATOR "Ninja")
#set(CTEST_CMAKE_GENERATOR "Visual Studio 15 2017")
#set(CTEST_CMAKE_GENERATOR_PLATFORM "x64") 
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
