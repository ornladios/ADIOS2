#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.12)
project(ADIOS2HelloBPStepsWriteReadHipExample)

set(maybe_standalone_project OFF)

if(NOT TARGET adios2_core)
  set(maybe_standalone_project ON)
  find_package(ADIOS2 REQUIRED COMPONENTS CXX)
endif()

enable_language(HIP)

# Needed for the hip cmake targets
find_package(hip REQUIRED)

add_executable(adios2_hello_bpStepsWriteReadHip bpStepsWriteReadHip.cpp)
target_link_libraries(adios2_hello_bpStepsWriteReadHip adios2::cxx hip::device)
set_source_files_properties(bpStepsWriteReadHip.cpp PROPERTIES LANGUAGE HIP)
if (maybe_standalone_project)
install(TARGETS adios2_hello_bpStepsWriteReadHip RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
