# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "cori.nersc.gov")
set(CTEST_BUILD_CONFIGURATION Release)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j10")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 1)

set(CTEST_BUILD_NAME "Linux-CrayCLE6-KNL_GCC_CrayMPICH")
set(dashboard_model Nightly)
set(CTEST_DASHBOARD_ROOT ${CMAKE_CURRENT_BINARY_DIR}/${CTEST_BUILD_NAME})


include(${CMAKE_CURRENT_LIST_DIR}/../EnvironmentModules.cmake)
module(purge)
module(load modules)
module(load craype)
module(load PrgEnv-gnu)
module(load craype-mic-knl)
module(load cray-mpich)
module(load cray-hdf5-parallel)
module(load cray-python)
module(load git)

set(ENV{CC}  cc)
set(ENV{CXX} CC)
set(ENV{FC}  ftn)

set(dashboard_cache "
#ADIOS2_USE_BZip2:STRING=ON
#ADIOS2_USE_DataMan:STRING=ON
ADIOS2_USE_Fortran:STRING=ON
ADIOS2_USE_HDF5:STRING=ON
ADIOS2_USE_MPI:STRING=ON
#ADIOS2_USE_Python:STRING=ON
#ADIOS2_USE_ZFP:STRING=ON
#ADIOS2_USE_ZeroMQ:STRING=ON

MPIEXEC_EXECUTABLE:FILEPATH=srun
MPIEXEC_MAX_NUMPROCS:STRING=
MPIEXEC_NUMPROC_FLAG:STRING=-n8;-c16
")

include(${CMAKE_CURRENT_LIST_DIR}/../adios_common.cmake)
