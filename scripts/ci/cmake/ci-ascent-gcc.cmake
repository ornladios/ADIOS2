# Client maintainer: chuck.atkins@kitware.com

find_package(EnvModules REQUIRED)

env_module(purge)
env_module(load git)
env_module(load gcc/8.1.1)
env_module(load hdf5)
env_module(load libfabric)
env_module(load python/3.7.0)
env_module(load zfp)
env_module(load zeromq)

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)

set(dashboard_cache "
ADIOS2_USE_BZip2:BOOL=OFF
ADIOS2_USE_Blosc:BOOL=OFF
ADIOS2_USE_DataMan:BOOL=ON
ADIOS2_USE_Fortran:BOOL=ON
ADIOS2_USE_HDF5:BOOL=ON
ADIOS2_USE_MPI:BOOL=OFF
ADIOS2_USE_Python:BOOL=OFF
ADIOS2_USE_SST:BOOL=ON
ADIOS2_USE_SZ:BOOL=OFF
ADIOS2_USE_ZeroMQ:STRING=ON
ADIOS2_USE_ZFP:BOOL=OFF

CMAKE_Fortran_FLAGS:STRING=-Wall
")

set(NCPUS 4)
set(CTEST_TEST_ARGS PARALLEL_LEVEL 8)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
