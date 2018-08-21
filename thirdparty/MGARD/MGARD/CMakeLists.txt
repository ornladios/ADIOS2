cmake_minimum_required(VERSION 3.6)

project(MGARD VERSION 0.0.0.1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib)

add_library(mgard
  include/mgard.h       src/mgard.cpp
  include/mgard_nuni.h  src/mgard_nuni.cpp
  include/mgard_capi.h  src/mgard_capi.cpp
)
target_include_directories(mgard
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
add_library(mgard::mgard ALIAS mgard)

find_package(ZLIB REQUIRED)
target_link_libraries(mgard PRIVATE ZLIB::ZLIB)

include(CTest)
if(BUILD_TESTING)
  add_executable(mgard_test src/mgard_test)
  target_link_libraries(mgard_test mgard)
endif()
