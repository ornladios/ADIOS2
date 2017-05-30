# Client maintainer: chuck.atkins@kitware.com
set(CTEST_BUILD_NAME "Linux-EL7_${COMP_ID}-${COMP_VER}_${MPI_ID}_ClangTidy")
set(CTEST_BUILD_CONFIGURATION Release)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j72")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 18)
#set(dashboard_model Nightly)
set(dashboard_root_name "Builds/${COMP_ID}_${COMP_VER}_${MPI_ID}_ClangTidy")

if(MPI_ID STREQUAL "NoMPI")
  set(MPI_CACHE "ADIOS_USE_MPI:BOOL=OFF")
else()
  set(MPI_CACHE "ADIOS_USE_MPI:BOOL=ON")
endif()

set(dashboard_cache "
CMAKE_C_CLANG_TIDY:FILEPATH=clang-tidy
CMAKE_CXX_CLANG_TIDY:FILEPATH=clang-tidy
CMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON

${MPI_CACHE}
ADIOS_USE_BZip2:BOOL=ON
")

include(${CMAKE_CURRENT_LIST_DIR}/adios_common.cmake)
