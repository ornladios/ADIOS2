# Client maintainer: chuck.atkins@kitware.com

find_package(EnvModules REQUIRED)

env_module(purge)
env_module(load pgi)

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
ADIOS2_USE_HDF5:BOOL=ON
ADIOS2_USE_MPI:BOOL=OFF
ADIOS2_USE_Python:BOOL=OFF
ADIOS2_USE_SZ:BOOL=ON
ADIOS2_USE_ZeroMQ:STRING=ON
ADIOS2_USE_ZFP:BOOL=ON
")

set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
