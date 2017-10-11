#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
    set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
    set(use_mpi USE_MPI_FOR_PYTHON_TESTS "TRUE")
endif()

python_add_test(NAME PythonBPWrite SCRIPT TestBPWriteTypes.py
    ${extra_test_args} ${use_mpi})
python_add_test(NAME TestOpenPythonEngineFromPython SCRIPT TestOpenPythonEngineFromPython.py
    ${extra_test_args} ${use_mpi}
    ADDTOPYPATH ${CMAKE_CURRENT_SOURCE_DIR})
python_add_test(NAME TestOpenInlinePythonEngineFromPython SCRIPT TestOpenInlinePythonEngineFromPython.py
    ${extra_test_args} ${use_mpi})

add_executable(TestPythonEngineFromCpp TestPythonEngineFromCpp.cpp)
target_link_libraries(TestPythonEngineFromCpp adios2 gtest gtest_main)

gtest_add_tests(TARGET TestPythonEngineFromCpp ${extra_test_args} TEST_LIST pyEngineTests)

set_property(TEST ${pyEngineTests} PROPERTY
    ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}:${ADIOS2_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}:$ENV{PYTHONPATH}"
)
