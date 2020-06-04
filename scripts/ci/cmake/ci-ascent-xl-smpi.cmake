# Client maintainer: chuck.atkins@kitware.com

find_package(EnvModules REQUIRED)

#env_module(purge)
env_module(load git)
env_module(load xl)
env_module(load spectrum-mpi)
env_module(load lsf-tools)
env_module(load hdf5)
env_module(load libfabric)
env_module(load python/3.7.0)
env_module(load zeromq)

set(ENV{CC}  xlc)
set(ENV{CXX} xlc++)
set(ENV{FC}  xlf)
set(ENV{CFLAGS}   "-qmaxmem=-1")
set(ENV{CXXFLAGS} "-qmaxmem=-1")
set(ENV{FFLAGS}   "-qmaxmem=-1")

find_program(JSRUN_EXECUTABLE jsrun)

set(dashboard_cache "
ADIOS2_USE_BZip2:BOOL=OFF
ADIOS2_USE_Blosc:BOOL=OFF
ADIOS2_USE_DataMan:BOOL=ON
ADIOS2_USE_Fortran:BOOL=ON
ADIOS2_USE_HDF5:BOOL=ON
ADIOS2_USE_MPI:BOOL=ON
ADIOS2_USE_Python:BOOL=OFF
ADIOS2_USE_SST:BOOL=ON
ADIOS2_USE_SZ:BOOL=OFF
ADIOS2_USE_ZeroMQ:STRING=ON
ADIOS2_USE_ZFP:BOOL=OFF

ADIOS2_RUN_MPI_MPMD_STATIC_NPROC_TESTS:BOOL=OFF
MPIEXEC_EXECUTABLE:FILEPATH=${JSRUN_EXECUTABLE}
MPIEXEC_MAX_NUMPROCS:STRING=12
MPIEXEC_EXTRA_FLAGS:STRING=-r6 -a1 -g1 -c7 -n12 $ENV{EXTERNAL_WORKDIR}/source/scripts/ci/scripts/mpmd-wrapper.sh JSM_NAMESPACE_RANK
MPIEXEC_NUMPROC_FLAG:STRING=-n
")

set(NCPUS 2)
set(CTEST_TEST_ARGS PARALLEL_LEVEL 1)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
