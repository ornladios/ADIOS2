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
  "Use a system-supplied Google Test framework" ON
  "BUILD_TESTING" OFF)
if(BUILD_TESTING)
  if(ADIOS_USE_SYSTEM_GOOGLETEST)
    find_package(GTest REQUIRED)
    if(NOT GTEST_FOUND)
      message(WARNING
        "Unable to find Google Test framework.  "
        "Using an internal version")
      set(ADIOS_USE_SYSTEM_GOOGLETEST OFF
        CACHE BOOL "Use a system-supplied Google Test framework" FORCE)
    endif()
  endif()
  if(NOT ADIOS_USE_SYSTEM_GOOGLETEST)
    add_subdirectory(googletest)
    find_package(GTest REQUIRED)
    add_dependencies(GTest::GTest googletest)
    add_dependencies(GTest::Main googletest)
  endif()
endif()
