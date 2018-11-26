#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestBPReadBigEndian TestBPReadBigEndian.cpp)
target_link_libraries(TestBPReadBigEndian adios2 gtest)

target_compile_definitions(TestBPReadBigEndian PRIVATE
  "BIG_ENDIAN_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestBPReadBigEndian MPI::MPI_C)
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
endif()

gtest_add_tests(TARGET TestBPReadBigEndian ${extra_test_args})

