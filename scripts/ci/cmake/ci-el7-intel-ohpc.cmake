# Client maintainer: chuck.atkins@kitware.com

find_package(EnvModules REQUIRED)

env_module(purge)
env_module(load intel)
env_module(load py3-numpy)
env_module(load hdf5)

set(ENV{CC}  icc)
set(ENV{CXX} icpc)
set(ENV{FC}  ifort)

set(dashboard_cache "
CMAKE_C_FLAGS:STRING=-Wall
CMAKE_CXX_FLAGS:STRING=-Wall
ADIOS2_USE_BZip2:BOOL=ON
ADIOS2_USE_Blosc:BOOL=ON
ADIOS2_USE_DataMan:BOOL=ON
ADIOS2_USE_Fortran:BOOL=ON
ADIOS2_USE_HDF5:BOOL=ON
ADIOS2_USE_MPI:BOOL=OFF
ADIOS2_USE_Python:BOOL=ON
ADIOS2_USE_SZ:BOOL=ON
ADIOS2_USE_ZeroMQ:STRING=ON
ADIOS2_USE_ZFP:STRING=ON

CMAKE_Fortran_FLAGS:STRING=-warn all -stand none
")

set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
