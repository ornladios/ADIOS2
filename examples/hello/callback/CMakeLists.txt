#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(hello_Callback1 helloCallback1.cpp)
  target_include_directories(hello_Callback1 PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_Callback1 ${MPI_C_LIBRARIES})
  
  add_executable(hello_Callback2 helloCallback2.cpp)
  target_include_directories(hello_Callback2 PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_Callback2 ${MPI_C_LIBRARIES})
  
else()
  #add_executable(hello_Callback helloBPZfpWrapper_nompi.cpp)
endif()

target_link_libraries(hello_Callback1 adios2)
target_link_libraries(hello_Callback2 adios2)
