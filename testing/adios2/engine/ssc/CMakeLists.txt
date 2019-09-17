#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(TestSscBase TestSscBase.cpp)
    target_link_libraries(TestSscBase adios2 gtest MPI::MPI_C)
    add_test(
        NAME TestSscBase
        COMMAND ${CMAKE_COMMAND}
        -DCMD1=TestSscBase.ssc
        -DCMD2=$<TARGET_FILE:TestSscBase>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake)
endif()

if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(TestSscNoAttributes TestSscNoAttributes.cpp)
    target_link_libraries(TestSscNoAttributes adios2 gtest MPI::MPI_C)
    add_test(
        NAME TestSscNoAttributes
        COMMAND ${CMAKE_COMMAND}
        -DCMD1=TestSscNoAttributes.ssc
        -DCMD2=$<TARGET_FILE:TestSscNoAttributes>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake)
endif()

if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(TestSscNoSelection TestSscNoSelection.cpp)
    target_link_libraries(TestSscNoSelection adios2 gtest MPI::MPI_C)
    add_test(
        NAME TestSscNoSelection
        COMMAND ${CMAKE_COMMAND}
        -DCMD1=TestSscNoSelection.ssc
        -DCMD2=$<TARGET_FILE:TestSscNoSelection>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake)
endif()

if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(TestSsc7d TestSsc7d.cpp)
    target_link_libraries(TestSsc7d adios2 gtest MPI::MPI_C)
    add_test(
        NAME TestSsc7d
        COMMAND ${CMAKE_COMMAND}
        -DCMD1=TestSsc7d.ssc
        -DCMD2=$<TARGET_FILE:TestSsc7d>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake)
endif()

