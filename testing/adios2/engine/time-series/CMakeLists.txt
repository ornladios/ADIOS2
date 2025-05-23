# ------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
# ------------------------------------------------------------------------------#
include(ADIOSFunctions)

set(BP3_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp3)
set(BP4_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp4)
set(BP5_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp5)

# set(HDF5_DIR ${CMAKE_CURRENT_BINARY_DIR}/hdf5)
file(MAKE_DIRECTORY ${BP3_DIR})
file(MAKE_DIRECTORY ${BP4_DIR})
file(MAKE_DIRECTORY ${BP5_DIR})

# file(MAKE_DIRECTORY ${HDF5_DIR})
macro(ts_gtest_add_tests_helper testname mpi)
    gtest_add_tests_helper(${testname} ${mpi} "" Engine.TimeSeries. .BP3
        WORKING_DIRECTORY ${BP3_DIR} EXTRA_ARGS "BP3"
    )
    gtest_add_tests_helper(${testname} ${mpi} "" Engine.TimeSeries. .BP4
        WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
    )
    gtest_add_tests_helper(${testname} ${mpi} "" Engine.TimeSeries. .BP5
        WORKING_DIRECTORY ${BP5_DIR} EXTRA_ARGS "BP5"
    )

    # # HDF5 does not work properly with Changing Shapes, so this test would fail too
    # if(ADIOS2_HAVE_HDF5)
    # gtest_add_tests_helper(${testname} ${mpi} "" Engine.TimeSeries. .HDF5
    # WORKING_DIRECTORY ${HDF5_DIR} EXTRA_ARGS "HDF5"
    # )
    # endif(ADIOS2_HAVE_HDF5)
endmacro()

ts_gtest_add_tests_helper(TimeSeries MPI_ALLOW)
