add_executable(writer writer.c dummy.c)
add_executable(reader reader.c dummy.c)

target_include_directories(writer PRIVATE .. ${MPI_C_INCLUDE_PATH} ../../source/adios2/toolkit/sst)
target_include_directories(reader PRIVATE .. ${MPI_C_INCLUDE_PATH} ../../source/adios2/toolkit/sst)

set_property(TARGET writer reader PROPERTY C_STANDARD 99)

target_link_libraries(writer fabric sst)
target_link_libraries(reader fabric sst)
