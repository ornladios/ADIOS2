#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)

add_executable(TestDataManP2P2DMemSelect TestDataManP2P2DMemSelect.cpp)
target_link_libraries(TestDataManP2P2DMemSelect adios2 gtest MPI::MPI_C)
gtest_add_tests(TARGET TestDataManP2P2DMemSelect)

add_executable(TestDataManP2P3DMemSelect TestDataManP2P3DMemSelect.cpp)
target_link_libraries(TestDataManP2P3DMemSelect adios2 gtest MPI::MPI_C)
gtest_add_tests(TARGET TestDataManP2P3DMemSelect)

add_executable(TestDataManP2P1D TestDataManP2P1D.cpp)
target_link_libraries(TestDataManP2P1D adios2 gtest MPI::MPI_C)
gtest_add_tests(TARGET TestDataManP2P1D)

add_executable(TestDataManP2PZfp2D TestDataManP2PZfp2D.cpp)
target_link_libraries(TestDataManP2PZfp2D adios2 gtest MPI::MPI_C)
gtest_add_tests(TARGET TestDataManP2PZfp2D)

add_executable(TestDataManP2PSz2D TestDataManP2PSz2D.cpp)
target_link_libraries(TestDataManP2PSz2D adios2 gtest MPI::MPI_C)
gtest_add_tests(TARGET TestDataManP2PSz2D)

add_executable(TestDataManSubscribe1D TestDataManSubscribe1D.cpp)
target_link_libraries(TestDataManSubscribe1D adios2 gtest MPI::MPI_C)
gtest_add_tests(TARGET TestDataManSubscribe1D)

add_executable(TestDataManMpiBase TestDataManMpiBase.cpp)
target_link_libraries(TestDataManMpiBase adios2 gtest MPI::MPI_C)
add_test(
    NAME DataManEngineTest.MpiBase
    COMMAND ${CMAKE_COMMAND}
    -DCMD1=TestDataManMpiBase.dataman
    -DCMD2=$<TARGET_FILE:TestDataManMpiBase>
    -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake)

endif()
