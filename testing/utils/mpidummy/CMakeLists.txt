add_executable(MPIDummyBarrier MPIDummyBarrier.c)
target_link_libraries(MPIDummyBarrier PRIVATE MPI::MPI_C)
