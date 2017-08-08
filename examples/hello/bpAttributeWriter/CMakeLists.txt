#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)

  add_executable(hello_bpAttributeWriter helloBPAttributeWriter.cpp)
  target_include_directories(hello_bpAttributeWriter PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_bpAttributeWriter ${MPI_C_LIBRARIES})
  
else()
  add_executable(hello_bpAttributeWriter helloBPAttributeWriter_nompi.cpp)
  
endif()

target_link_libraries(hello_bpAttributeWriter adios2)
