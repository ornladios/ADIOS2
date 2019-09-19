# Client maintainer: chuck.atkins@kitware.com

find_package(EnvModules REQUIRED)

env_module(purge)
env_module(load gnu8)
env_module(load py3-numpy)
env_module(load hdf5)

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)
set(ENV{CFLAGS} "-Werror -Wno-error=builtin-declaration-mismatch")
set(ENV{CXXFLAGS} "-Werror -Wno-error=builtin-declaration-mismatch")
set(ENV{FFLAGS} "-Werror -Wno-error=builtin-declaration-mismatch")

set(dashboard_cache "
ADIOS2_USE_BZip2:BOOL=ON
ADIOS2_USE_Blosc:BOOL=ON
ADIOS2_USE_DataMan:BOOL=ON
ADIOS2_USE_Fortran:BOOL=ON
ADIOS2_USE_HDF5:BOOL=ON
ADIOS2_USE_MPI:BOOL=OFF
ADIOS2_USE_Python:BOOL=ON
ADIOS2_USE_SZ:BOOL=ON
ADIOS2_USE_ZeroMQ:STRING=ON
ADIOS2_USE_ZFP:BOOL=ON

PYTHON_EXECUTABLE:FILEPATH=/usr/bin/python3.4
")

set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
