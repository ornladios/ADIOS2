set(ENV{CC}  cl)
set(ENV{CXX} cl)
set(ENV{CFLAGS} /WX)
set(ENV{CXXFLAGS} /WX)

set(dashboard_cache "
BUILD_TESTING:BOOL=ON
ADIOS2_BUILD_EXAMPLES:BOOL=ON

ADIOS2_USE_BZip2:BOOL=OFF
ADIOS2_USE_Fortran:BOOL=OFF
ADIOS2_USE_MPI:BOOL=ON
ADIOS2_USE_Python:BOOL=ON

Python_ROOT_DIR:PATH=$ENV{CMAKE_PREFIX_PATH}
Python_FIND_STRATEGY:STRING=LOCATION
Python_FIND_FRAMEWORK:STRING=FIRST
")

set(CTEST_TEST_TIMEOUT 500)
set(CTEST_CMAKE_GENERATOR "Visual Studio 17 2022")
set(CTEST_CMAKE_GENERATOR_PLATFORM "x64")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
# https://github.com/ornladios/ADIOS2/issues/4276
# https://github.com/ornladios/ADIOS2/issues/4279
# https://github.com/ornladios/ADIOS2/issues/4278
set(CTEST_TEST_ARGS
    EXCLUDE "Api.Python.FileReader|Api.Python.BPWriteTypesHighLevelAPI_HDF5|Api.Python.BPWriteTypesHighLevelAPI.MPI"
)
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
