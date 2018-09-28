# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "summitdev.ccs.ornl.gov")
set(CTEST_BUILD_CONFIGURATION Release)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j10")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 10)

set(CTEST_BUILD_NAME "Linux-EL7-PPC64LE_XL-13.1.5_NoMPI")
set(dashboard_model Nightly)
set(CTEST_DASHBOARD_ROOT ${CMAKE_CURRENT_BINARY_DIR}/${CTEST_BUILD_NAME})


include(${CMAKE_CURRENT_LIST_DIR}/../EnvironmentModules.cmake)
module(purge)
module(load git)
module(load xl)
module(load hdf5)

set(ENV{CC}  xlc)
set(ENV{CXX} xlC)
set(ENV{FC}  xlf)

set(dashboard_cache "
#ADIOS2_USE_BZip2:STRING=ON
#ADIOS2_USE_DataMan:STRING=ON
ADIOS2_USE_Fortran:STRING=ON
#ADIOS2_USE_HDF5:STRING=ON
#ADIOS2_USE_MPI:STRING=ON
#ADIOS2_USE_Python:STRING=ON
#ADIOS2_USE_ZFP:STRING=ON
#ADIOS2_USE_ZeroMQ:STRING=ON
")

include(${CMAKE_CURRENT_LIST_DIR}/../adios_common.cmake)
