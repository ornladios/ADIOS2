# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "aaargh.kitware.com")
set(CTEST_BUILD_CONFIGURATION Debug)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_COMMAND "cov-build --dir cov-int make")
set(CTEST_BUILD_FLAGS "-k -j36")

set(CTEST_BUILD_NAME "Linux-EL7_GCC7_Coverity")
set(dashboard_model Nightly)
set(CTEST_DASHBOARD_ROOT ${CMAKE_CURRENT_BINARY_DIR}/${CTEST_BUILD_NAME})

include(${CMAKE_CURRENT_LIST_DIR}/../EnvironmentModules.cmake)
module(purge)
module(load gnu7)
module(load py2-numpy)
module(load hdf5)

set(ENV{PATH} "/opt/coverity/cov-analysis-linux64-2017.07/bin:$ENV{PATH}")
set(dashboard_do_test OFF)
set(dashboard_track "Analysis")

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)

set(dashboard_cache "
ADIOS2_USE_BZip2:STRING=ON
ADIOS2_USE_DataMan:STRING=ON
ADIOS2_USE_Fortran:STRING=ON
ADIOS2_USE_HDF5:STRING=ON
ADIOS2_USE_MPI:STRING=OFF
ADIOS2_USE_Python:STRING=ON
ADIOS2_USE_SZ:STRING=ON
ZLIB_LIBRARY:FILEPATH=/usr/lib64/libz.so
ADIOS2_USE_ZFP:STRING=ON
ADIOS2_USE_ZeroMQ:STRING=ON

PYTHON_EXECUTABLE:FILEPATH=/usr/bin/python2.7
")

include(${CMAKE_CURRENT_LIST_DIR}/../adios_common.cmake)

message("Collecting Coverity scan results")
execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar cvfJ ${CTEST_BUILD_NAME}.tar.xz -- cov-int
  WORKING_DIRECTORY ${CTEST_DASHBOARD_ROOT}/ADIOS2-build
)

message("Determining source version")
execute_process(
  COMMAND git describe
  WORKING_DIRECTORY ${CTEST_DASHBOARD_ROOT}/ADIOS2
  OUTPUT_VARIABLE adios2_version
)

message("Uploading Coverity scan results")
execute_process(
  COMMAND curl
    --form token=$ENV{ADIOS2_COVERITY_SUBMIT_TOKEN}
    --form email=chuck.atkins@kitware.com
    --form version="${adios2_version}"
    --form description="Nightly NoMPI"
    --form file=@${CTEST_BUILD_NAME}.tar.xz
    "https://scan.coverity.com/builds?project=ornladios%2FADIOS2"
  WORKING_DIRECTORY ${CTEST_DASHBOARD_ROOT}/ADIOS2-build
)
