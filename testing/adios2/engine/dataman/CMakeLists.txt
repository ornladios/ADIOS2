#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestDataManP2P2DMemSelect TestDataManP2P2DMemSelect.cpp)
target_link_libraries(TestDataManP2P2DMemSelect adios2 gtest)
if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestDataManP2P2DMemSelect MPI::MPI_C)
endif()
gtest_add_tests(TARGET TestDataManP2P2DMemSelect)

add_executable(TestDataManP2P3DMemSelect TestDataManP2P3DMemSelect.cpp)
target_link_libraries(TestDataManP2P3DMemSelect adios2 gtest)
if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestDataManP2P3DMemSelect MPI::MPI_C)
endif()
gtest_add_tests(TARGET TestDataManP2P3DMemSelect)

add_executable(TestDataManP2P1D TestDataManP2P1D.cpp)
target_link_libraries(TestDataManP2P1D adios2 gtest)
if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestDataManP2P1D MPI::MPI_C)
endif()
gtest_add_tests(TARGET TestDataManP2P1D)

add_executable(TestDataManP2PZfp2D TestDataManP2PZfp2D.cpp)
target_link_libraries(TestDataManP2PZfp2D adios2 gtest)
if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestDataManP2PZfp2D MPI::MPI_C)
endif()
gtest_add_tests(TARGET TestDataManP2PZfp2D)

add_executable(TestDataManP2PSz2D TestDataManP2PSz2D.cpp)
target_link_libraries(TestDataManP2PSz2D adios2 gtest)
if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestDataManP2PSz2D MPI::MPI_C)
endif()
gtest_add_tests(TARGET TestDataManP2PSz2D)

add_executable(TestDataManP2PBzip2D TestDataManP2PBzip2D.cpp)
target_link_libraries(TestDataManP2PBzip2D adios2 gtest)
if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestDataManP2PBzip2D MPI::MPI_C)
endif()
gtest_add_tests(TARGET TestDataManP2PBzip2D)

add_executable(TestDataManSubscribe1D TestDataManSubscribe1D.cpp)
target_link_libraries(TestDataManSubscribe1D adios2 gtest)
if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestDataManSubscribe1D MPI::MPI_C)
endif()
gtest_add_tests(TARGET TestDataManSubscribe1D)

add_executable(TestDataManCallback1D TestDataManCallback1D.cpp)
target_link_libraries(TestDataManCallback1D adios2 gtest)
if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestDataManCallback1D MPI::MPI_C)
endif()
gtest_add_tests(TARGET TestDataManCallback1D)

