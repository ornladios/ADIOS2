#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(hello_datamanWriter helloDataManWriter.cpp)
  target_include_directories(hello_datamanWriter PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_datamanWriter ${MPI_C_LIBRARIES})
else()
  add_executable(hello_datamanWriter helloDataManWriter_nompi.cpp)
endif()
target_link_libraries(hello_datamanWriter adios2)
