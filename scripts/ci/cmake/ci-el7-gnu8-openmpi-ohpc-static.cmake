# Client maintainer: chuck.atkins@kitware.com

include(ProcessorCount)
ProcessorCount(NCPUS)
math(EXPR N2CPUS "${NCPUS}*2")

find_package(EnvModules REQUIRED)

env_module(purge)
env_module(load gnu8)
env_module(load py3-numpy)
env_module(load openmpi3)
env_module(load phdf5)
env_module(load py3-mpi4py)

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)

set(dashboard_cache "
BUILD_SHARED_LIBS:BOOL=OFF
CMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON

ADIOS2_BUILD_EXAMPLES:BOOL=OFF

ADIOS2_USE_BZip2:BOOL=OFF
ADIOS2_USE_Blosc:BOOL=OFF
ADIOS2_USE_Fortran:BOOL=ON
ADIOS2_USE_HDF5:BOOL=ON
ADIOS2_USE_MPI:BOOL=ON
ADIOS2_USE_Python:BOOL=OFF
ADIOS2_USE_SZ:BOOL=OFF
ADIOS2_USE_ZeroMQ:STRING=OFF
ADIOS2_USE_ZFP:BOOL=OFF

CMAKE_C_FLAGS:STRING=-Wall
CMAKE_CXX_FLAGS:STRING=-Wall
CMAKE_Fortran_FLAGS:STRING=-Wall

MPIEXEC_EXTRA_FLAGS:STRING=--allow-run-as-root --oversubscribe
MPIEXEC_MAX_NUMPROCS:STRING=${N2CPUS}
")

set(CTEST_TEST_ARGS
  PARALLEL_LEVEL 1
)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
