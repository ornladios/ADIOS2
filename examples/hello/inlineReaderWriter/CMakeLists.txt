#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(hello_inlineReaderWriter helloInlineReaderWriter.cpp)
  target_link_libraries(hello_inlineReaderWriter adios2 MPI::MPI_C)
endif()
