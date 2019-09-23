# Client maintainer: chuck.atkins@kitware.com

include(ProcessorCount)
ProcessorCount(NCPUS)
math(EXPR N2CPUS "${NCPUS}*2")

find_package(EnvModules REQUIRED)

env_module(purge)
env_module(load pgi)
env_module(load openmpi)

set(ENV{CC}  pgcc)
set(ENV{CXX} pgc++)
set(ENV{FC}  pgfortran)
#set(ENV{CFLAGS} -Werror)
set(ENV{CXXFLAGS} --brief_diagnostics)
#set(ENV{FFLAGS} "-warn errors")

set(dashboard_cache "
ADIOS2_USE_BZip2:BOOL=ON
ADIOS2_USE_Blosc:BOOL=OFF
ADIOS2_USE_DataMan:BOOL=ON
ADIOS2_USE_Fortran:BOOL=ON
ADIOS2_USE_HDF5:BOOL=OFF
ADIOS2_USE_MPI:BOOL=ON
ADIOS2_USE_Python:BOOL=OFF
ADIOS2_USE_SZ:BOOL=ON
ADIOS2_USE_ZeroMQ:STRING=ON
ADIOS2_USE_ZFP:BOOL=ON

MPIEXEC_MAX_NUMPROCS:STRING=${N2CPUS}
")

set(CTEST_TEST_ARGS PARALLEL_LEVEL 1)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
