cmake_minimum_required(VERSION 2.8.3)

# Note that this whole if block is for backwards compatibility and can be
# replaced with a single project(ATL VERSION A.B.C) if the minum required is
# raised to > 3.0
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
  project(ATL VERSION 2.2.1 LANGUAGES C)
else()
  project(ATL C)
  set(ATL_MAJOR_VERSION 2)
  set(ATL_MINOR_VERSION 2)
  set(ATL_PATCH_VERSION 1)
  set(ATL_VERSION
    ${ATL_MAJOR_VERSION}.${ATL_MINOR_VERSION}.${ATL_PATCH_VERSION})
endif()

# Some boilerplate to setup nice output directories
set(CMAKE_INSTALL_BINDIR bin CACHE STRING "Installation runtime subdirectory")
set(CMAKE_INSTALL_LIBDIR lib CACHE STRING "Installation library subdirectory")
set(CMAKE_INSTALL_INCLUDEDIR include
  CACHE STRING "Installation include subdirectory")
set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/atl
  CACHE STRING "Installation CMake subdirectory")
mark_as_advanced(CMAKE_INSTALL_BINDIR)
mark_as_advanced(CMAKE_INSTALL_LIBDIR)
mark_as_advanced(CMAKE_INSTALL_INCLUDEDIR)
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

# Setup shared library defaults.  If explicitly specified somehow, then default 
# to that.  Otherwise base the default on whether or not shared libs are even
# supported.
get_property(SHARED_LIBS_SUPPORTED GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)
cmake_dependent_option(BUILD_SHARED_LIBS
  "Build shared libraries (so/dylib/dll)." ${SHARED_LIBS_SUPPORTED}
  "SHARED_LIBS_SUPPORTED" OFF
)
mark_as_advanced(BUILD_SHARED_LIBS)

# Setup PIC defaults.  If explicitly specified somehow, then default 
# to that.  Otherwise base the default on whether or not shared libs are even
# supported.
if(DEFINED ATL_ENABLE_PIC)
  set(ATL_ENABLE_PIC_DEFAULT ${ATL_ENABLE_PIC})
elseif(DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
  set(ATL_ENABLE_PIC_DEFAULT ${CMAKE_POSITION_INDEPENDENT_CODE})
else()
  set(ATL_ENABLE_PIC_DEFAULT ${SHARED_LIBS_SUPPORTED})
endif()
cmake_dependent_option(ATL_ENABLE_PIC
  "Build with Position Independent Code" ${ATL_ENABLE_PIC_DEFAULT}
  "SHARED_LIBS_SUPPORTED" OFF
)
mark_as_advanced(ATL_ENABLE_PIC)
set(CMAKE_POSITION_INDEPENDENT_CODE ${ATL_ENABLE_PIC})
mark_as_advanced(CMAKE_POSITION_INDEPENDENT_CODE)

include(CheckTypeSize)
CHECK_TYPE_SIZE(double SIZEOF_DOUBLE)
CHECK_TYPE_SIZE(float SIZEOF_FLOAT)
CHECK_TYPE_SIZE(int SIZEOF_INT)
CHECK_TYPE_SIZE(short SIZEOF_SHORT)

include(CheckIncludeFiles)
CHECK_INCLUDE_FILES(malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES(sys/time.h HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILES(windows.h HAVE_WINDOWS_H)

include(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(fork unistd.h HAVE_FORK)

if(ATOM_SERVER_HOST)
  set(_atom_server_default "${ATOM_SERVER_HOST}")
else()
  set(_atom_server_default "atomhost.cercs.gatech.edu")
endif()
set(ATL_ATOM_SERVER_HOST "${_atom_server_default}" CACHE STRING
  "Default Atom server")
mark_as_advanced(ATL_ATOM_SERVER_HOST)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Set up include-directories
include_directories(
  "${PROJECT_SOURCE_DIR}"   # to find foo/foo.h
  "${PROJECT_BINARY_DIR}")  # to find foo/config.h
 
set(ATL_LIBRARY_PREFIX "" CACHE STRING
  "Prefix to prepend to the output library name")
mark_as_advanced(ATL_LIBRARY_PREFIX)

add_library(atl atom.c attr.c lookup3.c tclHash.c)
set_target_properties(atl PROPERTIES
  OUTPUT_NAME ${ATL_LIBRARY_PREFIX}atl
  VERSION ${ATL_VERSION}
  SOVERSION ${ATL_VERSION_MAJOR}
  PUBLIC_HEADER atl.h)
add_library(atl::atl ALIAS atl)

target_include_directories(atl PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if(NOT ANDROID)
  find_package(Threads)
  target_link_libraries(atl PRIVATE ${CMAKE_THREADS_LIBS_INIT})
endif()

# Setup pkgconfig
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/atl.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/atl.pc
  @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/atl.pc
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/atl-config.in
  ${CMAKE_CURRENT_BINARY_DIR}/atl-config
  @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/atl-config
  DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(FILES atl.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(TARGETS atl
  # IMPORTANT: Add the foo library to the "export-set"
  EXPORT atl-targets
  RUNTIME       DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT bin
  LIBRARY       DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT shlib
  ARCHIVE       DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT lib
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" COMPONENT dev)


option(ATL_LIBRARIES_ONLY "Don't built the ATL utility executables" OFF)
if(NOT ATL_LIBRARIES_ONLY)
  add_executable(atom_server atom_server.c)
  target_link_libraries(atom_server atl)

  add_executable(attr_dump attr_dump.c)
  target_link_libraries(attr_dump atl)

  install(TARGETS atom_server attr_dump
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT bin)
endif()

include(CTest)
mark_as_advanced(BUILD_TESTING)
if(BUILD_TESTING)
  add_executable(attr_test attr_test.c)
  target_link_libraries(attr_test atl)
  add_test(NAME atl_attr_test COMMAND attr_test)

  add_executable(atom_test atom_test.c)
  target_link_libraries(atom_test atl)

  add_executable(atom_check atom_check.c)
  target_link_libraries(atom_check atl)
  add_test(NAME atl_atom_check COMMAND atom_check) 
endif()

# Setup packaging and configs
 
# Add all targets to the build-tree export set
export(TARGETS atl NAMESPACE atl::
  FILE "${PROJECT_BINARY_DIR}/atl-targets.cmake")
configure_file(atl-config.cmake.in
  "${PROJECT_BINARY_DIR}/atl-config.cmake" @ONLY)
configure_file(atl-config-version.cmake.in
  "${PROJECT_BINARY_DIR}/atl-config-version.cmake" @ONLY)
 
# Install the atl-config.cmake and atl-config-version.cmake
install(FILES
  "${PROJECT_BINARY_DIR}/atl-config.cmake"
  "${PROJECT_BINARY_DIR}/atl-config-version.cmake"
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}" COMPONENT dev)
 
# Install the export set for use with the install-tree
install(EXPORT atl-targets NAMESPACE atl::
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}" COMPONENT dev)

set(CTEST_MEMORYCHECK_SUPPRESSIONS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/atl.supp)

# Display status message for important variables
option(ATL_QUIET "Suppress info output at the end of configure" OFF)
mark_as_advanced(ATL_QUIET)
if(NOT ATL_QUIET)
  message(STATUS )
  message( STATUS "----------------------------------------------------------------------------" )
  message( STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}" )
  message( STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}" )
  message( STATUS "ATL_ATOM_SERVER_HOST = ${ATL_ATOM_SERVER_HOST}" )
  message( STATUS "ATL_LIBRARIES_ONLY = ${ATL_LIBRARIES_ONLY}" )
  message( STATUS "BUILD_TESTING = ${BUILD_TESTING}" )
  message( STATUS "Change a value with: cmake -D<Variable>=<Value>" )
  message( STATUS "----------------------------------------------------------------------------" )
  message( STATUS )
endif()
