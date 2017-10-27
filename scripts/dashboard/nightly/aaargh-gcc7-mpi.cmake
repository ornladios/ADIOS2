# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "aaargh.kitware.com")
set(CTEST_BUILD_CONFIGURATION Release)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j36")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 1)

if(NOT (DEFINED MPI_MODULE AND DEFINED MPI_NAME))
  message("Using default OpenMPI (openmpi)")
  set(MPI_MODULE openmpi)
  set(MPI_NAME OpenMPI)
endif()

set(CTEST_BUILD_NAME "Linux-EL7_GCC7_${MPI_NAME}")
set(dashboard_model Nightly)
set(CTEST_DASHBOARD_ROOT ${CMAKE_CURRENT_BINARY_DIR}/${CTEST_BUILD_NAME})


include(${CMAKE_CURRENT_LIST_DIR}/../EnvironmentModules.cmake)
module(purge)
module(load gnu7)
module(load numpy)
module(load ${MPI_MODULE})
module(load phdf5)
module(load mpi4py)
module(load adios)

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)

set(dashboard_cache "
ADIOS2_USE_ADIOS1:STRING=ON
ADIOS2_USE_BZip2:STRING=ON
ADIOS2_USE_DataMan:STRING=ON
ADIOS2_USE_Fortran:STRING=ON
ADIOS2_USE_HDF5:STRING=ON
ADIOS2_USE_MPI:STRING=ON
ADIOS2_USE_Python:STRING=ON
ADIOS2_USE_ZFP:STRING=OFF
ADIOS2_USE_ZeroMQ:STRING=ON

MPIEXEC_MAX_NUMPROCS:STRING=4
")

include(${CMAKE_CURRENT_LIST_DIR}/../adios_common.cmake)
