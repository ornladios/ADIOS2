add_library(CLI11_wrapper INTERFACE)

if(ADIOS2_USE_EXTERNAL_CLI11)
  find_package(CLI11 REQUIRED)
  target_link_libraries(CLI11_wrapper INTERFACE
    $<BUILD_INTERFACE:CLI11::CLI11>
  )
else()
  target_sources(CLI11_wrapper INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/CLI/CLI.hpp>
  )
  target_include_directories(CLI11_wrapper INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )
endif()

install(TARGETS CLI11_wrapper EXPORT adios2Exports)
