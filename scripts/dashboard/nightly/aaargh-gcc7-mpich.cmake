# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "aaargh.kitware.com")
set(CTEST_BUILD_CONFIGURATION Debug)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j36")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 1)

set(CTEST_BUILD_NAME "Linux-EL7_GCC7_MPICH")
set(dashboard_model Nightly)
set(CTEST_DASHBOARD_ROOT ${CMAKE_CURRENT_BINARY_DIR}/${CTEST_BUILD_NAME})

include(${CMAKE_CURRENT_LIST_DIR}/../EnvironmentModules.cmake)
module(purge)
module(load gnu7)
module(load py2-numpy)
module(load mpich)
module(load phdf5)
module(load py2-mpi4py)
module(load adios)

module(load valgrind)
find_program(CTEST_MEMORYCHECK_COMMAND valgrind)
set(dashboard_do_memcheck ON)

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)

set(dashboard_cache "
ADIOS2_USE_BZip2:STRING=ON
ADIOS2_USE_DataMan:STRING=ON
ADIOS2_USE_Fortran:STRING=ON
ADIOS2_USE_HDF5:STRING=ON
ADIOS2_USE_MPI:STRING=ON
ADIOS2_USE_Python:STRING=ON
ADIOS2_USE_SZ:STRING=ON
ZLIB_LIBRARY:FILEPATH=/usr/lib64/libz.so
ADIOS2_USE_ZFP:STRING=ON
ADIOS2_USE_ZeroMQ:STRING=ON

MPIEXEC_MAX_NUMPROCS:STRING=16

PYTHON_EXECUTABLE:FILEPATH=/usr/bin/python2.7
")

include(${CMAKE_CURRENT_LIST_DIR}/../adios_common.cmake)
