#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(joinedArray_write joinedArray_write.cpp)
target_link_libraries(joinedArray_write adios2)

if(ADIOS2_HAVE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)
  target_include_directories(joinedArray_write PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(joinedArray_write ${MPI_C_LIBRARIES})
endif()
