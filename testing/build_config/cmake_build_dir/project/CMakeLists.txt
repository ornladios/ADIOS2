cmake_minimum_required(VERSION 3.6)

project(DummyProject C CXX)

find_package(ADIOS2 REQUIRED)

if(ADIOS2_HAVE_MPI)
  # Workaround for MPI forcing the link of C++ bindings
  add_definitions(-DOMPI_SKIP_MPICXX -DMPICH_SKIP_MPICXX)
endif()

add_executable(foo main.cxx)
target_link_libraries(foo adios2::adios2)
