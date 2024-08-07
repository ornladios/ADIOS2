include(ProcessorCount)
ProcessorCount(NCPUS)
math(EXPR N2CPUS "${NCPUS}*2")

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)

execute_process(
  COMMAND "python3-config" "--prefix"
  OUTPUT_VARIABLE PY_ROOT
  OUTPUT_STRIP_TRAILING_WHITESPACE)

set(dashboard_cache "
BUILD_TESTING:BOOL=ON
ADIOS2_BUILD_EXAMPLES:BOOL=ON

ADIOS2_USE_BZip2:BOOL=ON
ADIOS2_USE_Blosc2:BOOL=ON
ADIOS2_USE_DataMan:BOOL=ON
ADIOS2_USE_Fortran:BOOL=ON
ADIOS2_USE_HDF5:BOOL=ON
ADIOS2_USE_MPI:BOOL=ON
ADIOS2_USE_Python:BOOL=ON
ADIOS2_USE_SZ:BOOL=ON
ADIOS2_USE_ZeroMQ:STRING=ON
ADIOS2_USE_ZFP:BOOL=ON

Python_ROOT_DIR:PATH=${PY_ROOT}
Python_FIND_STRATEGY:STRING=LOCATION
Python_FIND_FRAMEWORK:STRING=FIRST

CMAKE_C_COMPILER_LAUNCHER=ccache
CMAKE_CXX_COMPILER_LAUNCHER=ccache
CMAKE_C_FLAGS:STRING=-Wall
CMAKE_CXX_FLAGS:STRING=-Wall
CMAKE_Fortran_FLAGS:STRING=-Wall
OpenMP_gomp_LIBRARY:FILEPATH=/spack/var/spack/environments/adios2-ci-serial/.spack-env/view/lib/libgomp.so.1

MPIEXEC_MAX_NUMPROCS:STRING=${N2CPUS}
")

# TODO: The Kill* and PreciousTimeStep tests fail (due to timeout) when
# TODO: adios2 is built "--with-device=ch3:sock:tcp".  Once this is fixed
# TODO:  in the mpi_dp, we can re-enable these tests.
set(CTEST_TEST_ARGS EXCLUDE "KillReader|KillWriter|PreciousTimestep")

set(CTEST_CMAKE_GENERATOR "Ninja")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
