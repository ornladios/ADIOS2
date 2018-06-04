#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestBPWriteReadADIOS2 TestBPWriteReadADIOS2.cpp)
target_link_libraries(TestBPWriteReadADIOS2 adios2 gtest)

add_executable(TestBPWriteReadAsStreamADIOS2 TestBPWriteReadAsStreamADIOS2.cpp)
target_link_libraries(TestBPWriteReadAsStreamADIOS2 adios2 gtest)

add_executable(TestBPWriteReadAsStreamADIOS2_Threads
  TestBPWriteReadAsStreamADIOS2_Threads.cpp
)
target_link_libraries(TestBPWriteReadAsStreamADIOS2_Threads adios2 gtest)

add_executable(TestBPWriteReadAttributesADIOS2
  TestBPWriteReadAttributesADIOS2.cpp
)
target_link_libraries(TestBPWriteReadAttributesADIOS2 adios2 gtest)

add_executable(TestStreamWriteReadHighLevelAPI
  TestStreamWriteReadHighLevelAPI.cpp
)
target_link_libraries(TestStreamWriteReadHighLevelAPI adios2 gtest)

add_executable(TestBPWriteFlushRead TestBPWriteFlushRead.cpp)
target_link_libraries(TestBPWriteFlushRead adios2 gtest)

add_executable(TestBPWriteMultiblockRead TestBPWriteMultiblockRead.cpp)
target_link_libraries(TestBPWriteMultiblockRead adios2 gtest)

if(ADIOS2_HAVE_MPI)

  target_link_libraries(TestBPWriteReadADIOS2 MPI::MPI_C)
  target_link_libraries(TestBPWriteReadAsStreamADIOS2 MPI::MPI_C)
  target_link_libraries(TestBPWriteReadAsStreamADIOS2_Threads MPI::MPI_C)
  target_link_libraries(TestBPWriteReadAttributesADIOS2 MPI::MPI_C)
  target_link_libraries(TestStreamWriteReadHighLevelAPI MPI::MPI_C)
  target_link_libraries(TestBPWriteFlushRead MPI::MPI_C)
  target_link_libraries(TestBPWriteMultiblockRead MPI::MPI_C)
  
  add_executable(TestBPWriteAggregateRead TestBPWriteAggregateRead.cpp)
  target_link_libraries(TestBPWriteAggregateRead
    adios2 gtest_interface MPI::MPI_C
  )
  
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
  gtest_add_tests(TARGET TestBPWriteAggregateRead ${extra_test_args})
  
endif()

gtest_add_tests(TARGET TestBPWriteReadADIOS2 ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteReadAsStreamADIOS2 ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteReadAsStreamADIOS2_Threads ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteReadAttributesADIOS2 ${extra_test_args})
gtest_add_tests(TARGET TestStreamWriteReadHighLevelAPI ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteFlushRead ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteMultiblockRead ${extra_test_args})
  
if (ADIOS2_HAVE_ADIOS1)
  add_executable(TestBPWriteRead TestBPWriteRead.cpp)
  target_link_libraries(TestBPWriteRead adios2 gtest_interface adios1::adios)

  add_executable(TestBPWriteReadAttributes TestBPWriteReadAttributes.cpp)
  target_link_libraries(TestBPWriteReadAttributes
    adios2 gtest_interface adios1::adios
  )

  add_executable(TestBPWriteReadstdio TestBPWriteReadstdio.cpp)
  target_link_libraries(TestBPWriteReadstdio
    adios2 gtest_interface adios1::adios
  )

  add_executable(TestBPWriteReadfstream TestBPWriteReadfstream.cpp)
  target_link_libraries(TestBPWriteReadfstream
    adios2 gtest_interface adios1::adios
  )

  add_executable(TestBPWriteProfilingJSON TestBPWriteProfilingJSON.cpp)
  target_link_libraries(TestBPWriteProfilingJSON
    adios2 gtest_interface adios2::thirdparty::nlohmann_json
  )

  if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestBPWriteRead MPI::MPI_C)
#    target_link_libraries(TestBPWriteReadAttributes MPI::MPI_C)
    target_link_libraries(TestBPWriteReadstdio MPI::MPI_C)
    target_link_libraries(TestBPWriteReadfstream MPI::MPI_C)
    target_link_libraries(TestBPWriteProfilingJSON MPI::MPI_C)
  endif()

  gtest_add_tests(TARGET TestBPWriteRead ${extra_test_args})
#  gtest_add_tests(TARGET TestBPWriteReadAttributes ${extra_test_args})
  gtest_add_tests(TARGET TestBPWriteReadstdio ${extra_test_args})
  gtest_add_tests(TARGET TestBPWriteReadfstream ${extra_test_args})
  gtest_add_tests(TARGET TestBPWriteProfilingJSON ${extra_test_args})
endif()
