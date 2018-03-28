cmake_minimum_required(VERSION 3.6)

project(ENET VERSION 1.3.13 LANGUAGES C)

# Some boilerplate to setup nice output directories
set(CMAKE_INSTALL_BINDIR bin CACHE STRING "Installation runtime subdirectory")
set(CMAKE_INSTALL_LIBDIR lib CACHE STRING "Installation library subdirectory")
set(CMAKE_INSTALL_INCLUDEDIR include
  CACHE STRING "Installation include subdirectory")
set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/enet
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
if(DEFINED ENET_ENABLE_PIC)
  set(ENET_ENABLE_PIC_DEFAULT ${ENET_ENABLE_PIC})
elseif(DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
  set(ENET_ENABLE_PIC_DEFAULT ${CMAKE_POSITION_INDEPENDENT_CODE})
else()
  set(ENET_ENABLE_PIC_DEFAULT ${SHARED_LIBS_SUPPORTED})
endif()
cmake_dependent_option(ENET_ENABLE_PIC
  "Build with Position Independent Code" ${ENET_ENABLE_PIC_DEFAULT}
  "SHARED_LIBS_SUPPORTED" OFF
)
mark_as_advanced(ENET_ENABLE_PIC)
set(CMAKE_POSITION_INDEPENDENT_CODE ${ENET_ENABLE_PIC})
mark_as_advanced(CMAKE_POSITION_INDEPENDENT_CODE)

include(CheckFunctionExists)
check_function_exists(getaddrinfo HAS_GETADDRINFO)
check_function_exists(getnameinfo HAS_GETNAMEINFO)
check_function_exists(gethostbyaddr_r HAS_GETHOSTBYADDR_R)
check_function_exists(gethostbyname_r HAS_GETHOSTBYNAME_R)
check_function_exists(poll HAS_POLL)
check_function_exists(fcntl HAS_FCNTL)
check_function_exists(inet_pton HAS_INET_PTON)
check_function_exists(inet_ntop HAS_INET_NTOP)

include(CheckStructHasMember)
check_struct_has_member("struct msghdr" msg_flags sys/socket.h HAS_MSGHDR_FLAGS)

include(CheckCSourceCompiles)
check_c_source_compiles([=[
#include <sys/socket.h>
int main() { return sizeof(socklen_t); } ]=]
  HAS_SOCKLEN_T
)

set(ENET_LIBRARY_PREFIX "" CACHE STRING
  "Prefix to prepend to the output library name")
mark_as_advanced(ENET_LIBRARY_PREFIX)

add_library(enet
  callbacks.c
  compress.c
  host.c
  list.c
  packet.c
  peer.c
  protocol.c
)
set_target_properties(enet PROPERTIES
  OUTPUT_NAME ${ENET_LIBRARY_PREFIX}enet
  VERSION ${ENET_VERSION}
  SOVERSION ${ENET_VERSION_MAJOR}
)
add_library(enet::enet ALIAS enet)

target_compile_definitions(enet PRIVATE
  $<$<BOOL:${HAS_GETADDRINFO}>:HAS_GETADDRINFO>
  $<$<BOOL:${HAS_GETNAMEINFO}>:HAS_GETNAMEINFO>
  $<$<BOOL:${HAS_GETHOSTBYADDR_R}>:HAS_GETHOSTBYADDR_R>
  $<$<BOOL:${HAS_GETHOSTBYNAME_R}>:HAS_GETHOSTBYNAME_R>
  $<$<BOOL:${HAS_POLL}>:HAS_POLL>
  $<$<BOOL:${HAS_FCNTL}>:HAS_FCNTL>
  $<$<BOOL:${HAS_INET_PTON}>:HAS_INET_PTON>
  $<$<BOOL:${HAS_INET_NTOP}>:HAS_INET_NTOP>
  $<$<BOOL:${HAS_MSGHDR_FLAGS}>:HAS_MSGHDR_FLAGS>
  $<$<BOOL:${HAS_SOCKLEN_T}>:HAS_SOCKLEN_T>
)

if(UNIX)
  target_sources(enet PRIVATE unix.c)
elseif(WIN32)
  target_sources(enet PRIVATE win32.c)
endif()
target_include_directories(enet PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(DIRECTORY include/enet
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  FILES_MATCHING PATTERN "*.h"
)
install(TARGETS enet
  EXPORT enet-targets
  RUNTIME       DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY       DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE       DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

# Setup packaging and configs
 
# Add all targets to the build-tree export set
export(TARGETS enet NAMESPACE enet::
  FILE "${PROJECT_BINARY_DIR}/enet-targets.cmake")
configure_file(enet-config.cmake.in
  "${PROJECT_BINARY_DIR}/enet-config.cmake" @ONLY)
configure_file(enet-config-version.cmake.in
  "${PROJECT_BINARY_DIR}/enet-config-version.cmake" @ONLY)
 
# Install the enet-config.cmake and enet-config-version.cmake
install(FILES
  "${PROJECT_BINARY_DIR}/enet-config.cmake"
  "${PROJECT_BINARY_DIR}/enet-config-version.cmake"
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")
 
# Install the export set for use with the install-tree
install(EXPORT enet-targets NAMESPACE enet::
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")
