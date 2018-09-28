# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "AppVeyor")
set(CTEST_BUILD_CONFIGURATION Release)
set(CTEST_CMAKE_GENERATOR "Ninja")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 4)

set(dashboard_model Experimental)
set(dashboard_binary_name "build_ninja")
set(dashboard_track "Continuous Integration")

set(CTEST_GIT_COMMAND "git.exe")
set(CTEST_UPDATE_VERSION_ONLY TRUE)
set(CTEST_SOURCE_DIRECTORY "$ENV{APPVEYOR_BUILD_FOLDER}")
set(CTEST_DASHBOARD_ROOT "C:/projects/adios2build")

set(ENV{CFLAGS} /WX)
set(ENV{CXXFLAGS} /WX)

set(dashboard_cache "
ADIOS2_USE_BZip2:STRING=OFF
ADIOS2_USE_DataMan:STRING=OFF
ADIOS2_USE_Fortran:STRING=OFF
ADIOS2_USE_HDF5:STRING=OFF
ADIOS2_USE_MPI:STRING=OFF
ADIOS2_USE_Python:STRING=OFF
ADIOS2_USE_ZFP:STRING=OFF
ADIOS2_USE_ZeroMQ:STRING=OFF
")

include(${CMAKE_CURRENT_LIST_DIR}/../dashboard/adios_common.cmake)
