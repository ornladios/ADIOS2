# Client maintainer: chuck.atkins@kitware.com

include(ProcessorCount)
ProcessorCount(NCPUS)
math(EXPR N2CPUS "${NCPUS}*2")

find_package(EnvModules REQUIRED)

env_module(purge)
env_module(load intel)
env_module(load py3-numpy)
env_module(load openmpi3)
env_module(load phdf5)
env_module(load py3-mpi4py)

set(ENV{CC}  icc)
set(ENV{CXX} icpc)
set(ENV{FC}  ifort)

set(dashboard_cache "
ADIOS2_USE_BZip2:BOOL=ON
ADIOS2_USE_Blosc:BOOL=ON
ADIOS2_USE_DataMan:BOOL=ON
ADIOS2_USE_Fortran:BOOL=ON
ADIOS2_USE_HDF5:BOOL=ON
ADIOS2_USE_MPI:BOOL=ON
ADIOS2_USE_Python:BOOL=ON
ADIOS2_USE_SZ:BOOL=ON
ADIOS2_USE_ZeroMQ:STRING=ON
ADIOS2_USE_ZFP:STRING=ON

CMAKE_C_FLAGS:STRING=-Wall
CMAKE_CXX_FLAGS:STRING=-Wall
CMAKE_Fortran_FLAGS:STRING=-warn all -stand none

MPIEXEC_EXTRA_FLAGS:STRING=--allow-run-as-root --oversubscribe
MPIEXEC_MAX_NUMPROCS:STRING=${N2CPUS}
")

set(CTEST_TEST_ARGS PARALLEL_LEVEL 1)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
