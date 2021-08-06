# Client maintainer: chuck.atkins@kitware.com
set(ENV{CC}  clang)
set(ENV{CXX} clang++)
set(ENV{FC}  gfortran)

include(ProcessorCount)
ProcessorCount(NCPUS)
math(EXPR N2CPUS "${NCPUS}*2")

set(dashboard_cache "
ADIOS2_USE_Fortran:BOOL=ON
ADIOS2_USE_MPI:STRING=ON

CMAKE_C_FLAGS:STRING=-Wall
CMAKE_CXX_FLAGS:STRING=-Wall
CMAKE_Fortran_FLAGS:STRING=-Wall

MPIEXEC_EXTRA_FLAGS:STRING=--oversubscribe
MPIEXEC_MAX_NUMPROCS:STRING=4
")

set(ENV{MACOSX_DEPLOYMENT_TARGET} "10.14")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 1)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
