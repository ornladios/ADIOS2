add_executable(metadata_complexity metadata_complexity.cpp)
target_link_libraries(metadata_complexity adios2::cxx11 benchmark::benchmark benchmark::benchmark_main)

# Some trickery needs to happen to get this to work with MPI; see VTK-m's work therein.
#if(ADIOS2_HAVE_MPI)
#  add_executable(metadata_complexity_mpi metadata_complexity.cpp)
#  target_link_libraries(metadata_complexity_mpi adios2::cxx11_mpi benchmark::benchmark MPI::MPI_C)
#endif()
