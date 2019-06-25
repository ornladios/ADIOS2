#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#


if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(SscBase SscBase.cpp)
    target_link_libraries(SscBase adios2 gtest MPI::MPI_C)
    add_test(
        NAME SscBase
        COMMAND ${CMAKE_COMMAND}
        -DCMD1=SscBase.ssc
        -DCMD2=$<TARGET_FILE:SscBase>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake)
endif()

if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(SscNoAttributes SscNoAttributes.cpp)
    target_link_libraries(SscNoAttributes adios2 gtest MPI::MPI_C)
    add_test(
        NAME SscNoAttributes
        COMMAND ${CMAKE_COMMAND}
        -DCMD1=SscNoAttributes.ssc
        -DCMD2=$<TARGET_FILE:SscNoAttributes>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake)
endif()

if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(SscNoSelection SscNoSelection.cpp)
    target_link_libraries(SscNoSelection adios2 gtest MPI::MPI_C)
    add_test(
        NAME SscNoSelection
        COMMAND ${CMAKE_COMMAND}
        -DCMD1=SscNoSelection.ssc
        -DCMD2=$<TARGET_FILE:SscNoSelection>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake)
endif()

if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(Ssc7d Ssc7d.cpp)
    target_link_libraries(Ssc7d adios2 gtest MPI::MPI_C)
    add_test(
        NAME Ssc7d
        COMMAND ${CMAKE_COMMAND}
        -DCMD1=Ssc7d.ssc
        -DCMD2=$<TARGET_FILE:Ssc7d>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/RunTest.cmake)
endif()

