#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

find_package(HDF5 REQUIRED)

add_executable(TestHDF5WriteRead TestHDF5WriteRead.cpp)
target_include_directories(TestHDF5WriteRead PRIVATE ${HDF5_C_INCLUDE_DIRS})
target_link_libraries(TestHDF5WriteRead adios2 gtest ${HDF5_C_LIBRARIES})

add_test(NAME adios2::engine::hdf5::write_read COMMAND TestHDF5WriteRead)
