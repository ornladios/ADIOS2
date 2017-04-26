#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# MPI versions of the test are not properly implemented at the moment
if(NOT ADIOS_USE_MPI)
  find_package(ADIOS1 COMPONENTS sequential REQUIRED)

  add_executable(TestADIOS1WriteRead TestADIOS1WriteRead.cpp)
  target_link_libraries(TestADIOS1WriteRead adios2 gtest adios1::adios)

  add_test(NAME adios2::engine::adios1::write_read COMMAND TestADIOS1WriteRead)
endif()
