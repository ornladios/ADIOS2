# Client maintainer: chuck.atkins@kitware.com

include(ProcessorCount)
ProcessorCount(NCPUS)
math(EXPR N2CPUS "${NCPUS}*2")

find_package(EnvModules REQUIRED)

env_module(purge)
env_module(load gnu8)
env_module(load py3-numpy)
env_module(load openmpi3)
env_module(load py3-mpi4py)

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)
set(ENV{CFLAGS} "-Werror -Wno-error=builtin-declaration-mismatch")
set(ENV{CXXFLAGS} "-Werror -Wno-error=builtin-declaration-mismatch")
set(ENV{FFLAGS} "-Werror -Wno-error=builtin-declaration-mismatch")

set(ENV{CMAKE_PREFIX_PATH} "/opt/libfabric/1.6.0:/opt/hdf5/1.10.4:$ENV{CMAKE_PREFIX_PATH}")

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
ADIOS2_USE_ZFP:BOOL=ON

MPIEXEC_MAX_NUMPROCS:STRING=${N2CPUS}
PYTHON_EXECUTABLE:FILEPATH=/usr/bin/python3.4
")

set(CTEST_TEST_ARGS PARALLEL_LEVEL 1)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
