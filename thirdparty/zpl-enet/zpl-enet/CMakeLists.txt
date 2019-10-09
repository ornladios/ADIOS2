cmake_minimum_required(VERSION 3.3)

project(ZPL-ENET VERSION 1.3.15 LANGUAGES C)

message (STATUS "in subdir")

# Some boilerplate to setup nice output directories
include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/zpl-enet
  CACHE STRING "Installation CMake subdirectory")
mark_as_advanced(CMAKE_INSTALL_CMAKEDIR)

if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
endif()

# Default to a RelWithDebInfo build if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE RelWithDebugInfo)
endif()

include(CMakeDependentOption)


if(MSVC)
	add_definitions(-W3)
endif()

set (ZPL-ENET_SHARED ON)
if (ZPL-ENET_STATIC)
    add_library(zpl-enet_static STATIC test/library.c)

    if (WIN32)
        target_link_libraries(zpl-enet_static winmm ws2_32)
    endif()
    set (ZPL-ENET_LIBRARY zpl-enet_static)
endif()

if (ZPL-ENET_SHARED)
    add_definitions(-DZPL_ENET_DLL)
    add_library(zpl-enet SHARED test/library.c)

    if (WIN32)
        target_link_libraries(zpl-enet winmm ws2_32)
    endif()
    set (ZPL-ENET_LIBRARY zpl-enet)
set_target_properties(zpl-enet PROPERTIES
  OUTPUT_NAME ${ZPL-ENET_LIBRARY_PREFIX}zpl-enet
  VERSION ${ZPL-ENET_VERSION}
  SOVERSION ${ZPL-ENET_VERSION_MAJOR}
)
add_library(zpl-enet::zpl-enet ALIAS zpl-enet)

endif()

target_include_directories(${ZPL-ENET_LIBRARY} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if (ZPL-ENET_TEST)
    add_executable(zpl-enet_test test/build.c)

    if (WIN32)
        target_link_libraries(zpl-enet_test winmm ws2_32)
    endif()
endif()
option(ZPL-ENET_INSTALL_HEADERS "Install zpl-enet header files" ON)
mark_as_advanced(ZPL-ENET_INSTALL_HEADERS)
if(ZPL-ENET_INSTALL_HEADERS)
  install(FILES include/enet.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  )
endif()
# Setup pkgconfig
option(ZPL-ENET_INSTALL_PKGCONFIG "Install zpl-enet pkgconfig files" ON)
message(STATUS "Doing pakgconfig install" ${ZPL-ENET_INSTALL_PKGCONFIG})
mark_as_advanced(ZPL-ENET_INSTALL_PKGCONFIG)
if(ZPL-ENET_INSTALL_PKGCONFIG)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/zpl-enet.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/zpl-enet.pc
    @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/zpl-enet.pc
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/zpl-enet-config.in
    ${CMAKE_CURRENT_BINARY_DIR}/zpl-enet-config
    @ONLY)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/zpl-enet-config
    DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

install(TARGETS ${ZPL-ENET_LIBRARY}
  EXPORT zpl-enet-targets
  RUNTIME       DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY       DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE       DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

# Setup packaging and configs

# Add all targets to the build-tree export set
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/zpl-enet-config-version.cmake"
  COMPATIBILITY SameMajorVersion)
configure_file(zpl-enet-config.cmake.in
  "${PROJECT_BINARY_DIR}/zpl-enet-config.cmake" @ONLY)
export(TARGETS ${ZPL-ENET_LIBRARY} NAMESPACE zpl-enet::
  FILE "${PROJECT_BINARY_DIR}/zpl-enet-targets.cmake")

# Install the zpl-enet-config.cmake and zpl-enet-config-version.cmake
install(FILES
  "${PROJECT_BINARY_DIR}/zpl-enet-config.cmake"
  "${PROJECT_BINARY_DIR}/zpl-enet-config-version.cmake"
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")

# Install the export set for use with the install-tree
install(EXPORT zpl-enet-targets NAMESPACE zpl-enet::
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")

