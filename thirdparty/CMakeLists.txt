set(ADIOS_THIRDPARTY_DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/downloads
  CACHE PATH "Download directory for third party dependencies")

include(ExternalProject)

# Boilerplate settings for a common external project infrastructure
set(EP_ARGS
  DOWNLOAD_DIR ${ADIOS_THIRDPARTY_DOWNLOAD_DIR}
  PREFIX       .
  SOURCE_DIR   source
  BINARY_DIR   build
  STAMP_DIR    stamp
  INSTALL_DIR  install)

# Use Google Test for a unit testing framework
cmake_dependent_option(ADIOS_USE_SYSTEM_GOOGLETEST
  "Use a system-supplied Google Test framework" OFF
  "BUILD_TESTING" OFF)
if(BUILD_TESTING)
  if(NOT ADIOS_USE_SYSTEM_GOOGLETEST)
    add_subdirectory(googletest)
  endif()
  find_package(GTest REQUIRED)
  if(NOT ADIOS_USE_SYSTEM_GOOGLETEST)
    add_dependencies(GTest::GTest googletest)
    add_dependencies(GTest::Main googletest)
  endif()
endif()
