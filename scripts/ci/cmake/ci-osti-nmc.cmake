# Client maintainer: omar.padron@kitware.com

find_package(EnvModules REQUIRED)

env_module(purge)
env_module(load gcc/8.3.0)
env_module(load python)
env_module(load hdf5)
env_module(load libzmq)
env_module(load openmpi)

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)
set(ENV{CFLAGS} "-Werror -Wno-error=builtin-declaration-mismatch")
set(ENV{CXXFLAGS} "-Werror -Wno-error=builtin-declaration-mismatch")
set(ENV{FFLAGS} "-Werror -Wno-error=builtin-declaration-mismatch")

find_program(SRUN_EXECUTABLE srun)
set(dashboard_cache "
ADIOS2_USE_BZip2:STRING=ON
ADIOS2_USE_DataMan:STRING=ON
ADIOS2_USE_Fortran:STRING=ON
ADIOS2_USE_HDF5:STRING=ON
ADIOS2_USE_MPI:STRING=ON
ADIOS2_USE_Python:STRING=OFF

MPIEXEC_EXECUTABLE:FILEPATH=${SRUN_EXECUTABLE}
MPIEXEC_MAX_NUMPROCS:STRING=4
MPIEXEC_NUMPROC_FLAG:STRING=-n
MPIEXEC_EXTRA_FLAGS:STRING=-O
")

set(CTEST_TEST_ARGS PARALLEL_LEVEL 1)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
