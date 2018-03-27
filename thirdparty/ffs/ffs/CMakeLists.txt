cmake_minimum_required(VERSION 2.8.3)

# Note that this whole if block is for backwards compatibility and can be
# replaced with a single project(ATL VERSION A.B.C) if the minum required is
# raised to > 3.0
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
  project(FFS VERSION 1.5.1)
else()
  project(FFS)
  set(FFS_VERSION_MAJOR 1)
  set(FFS_VERSION_MINOR 5)
  set(FFS_VERSION_PATCH 1)
  set(FFS_VERSION
    ${FFS_VERSION_MAJOR}.${FFS_VERSION_MINOR}.${FFS_VERSION_PATCH})
endif()

# Some boilerplate to setup nice output directories
set(CMAKE_INSTALL_BINDIR bin CACHE STRING "Installation runtime subdirectory")
set(CMAKE_INSTALL_LIBDIR lib CACHE STRING "Installation library subdirectory")
set(CMAKE_INSTALL_INCLUDEDIR include
  CACHE STRING "Installation include subdirectory")
if(WIN32 AND NOT CYGWIN)
  set(CMAKE_INSTALL_CMAKEDIR CMake
    CACHE STRING "Installation CMake subdirectory")
else()
	set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/ffs
    CACHE STRING "Installation CMake subdirectory")
endif()
mark_as_advanced(CMAKE_INSTALL_BINDIR)
mark_as_advanced(CMAKE_INSTALL_LIBDIR)
mark_as_advanced(CMAKE_INSTALL_INCLUDEDIR)
mark_as_advanced(CMAKE_INSTALL_CMAKEDIR)

list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake)
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

list(INSERT CMAKE_PREFIX_PATH 0 ${CMAKE_INSTALL_PREFIX})

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
if(DEFINED FFS_ENABLE_PIC)
  set(FFS_ENABLE_PIC_DEFAULT ${FFS_ENABLE_PIC})
elseif(DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
  set(FFS_ENABLE_PIC_DEFAULT ${CMAKE_POSITION_INDEPENDENT_CODE})
else()
  set(FFS_ENABLE_PIC_DEFAULT ${SHARED_LIBS_SUPPORTED})
endif()
cmake_dependent_option(FFS_ENABLE_PIC
  "Build with Position Independent Code" ${FFS_ENABLE_PIC_DEFAULT}
  "SHARED_LIBS_SUPPORTED" OFF
)
mark_as_advanced(FFS_ENABLE_PIC)
set(CMAKE_POSITION_INDEPENDENT_CODE ${FFS_ENABLE_PIC})
mark_as_advanced(CMAKE_POSITION_INDEPENDENT_CODE)

include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckTypeSize)
include(CheckStructHasMember)
include(CMakeFindDependencyMacro)

CHECK_TYPE_SIZE("double" SIZEOF_DOUBLE)
CHECK_TYPE_SIZE("float" SIZEOF_FLOAT)
CHECK_TYPE_SIZE("int" SIZEOF_INT)
CHECK_TYPE_SIZE("off_t" SIZEOF_OFF_T)
CHECK_TYPE_SIZE("long" SIZEOF_LONG)
CHECK_TYPE_SIZE("long double" SIZEOF_LONG_DOUBLE)
CHECK_TYPE_SIZE("long long" SIZEOF_LONG_LONG)
CHECK_TYPE_SIZE("size_t" SIZEOF_SIZE_T)

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

set (CMAKE_MACOSX_RPATH 1)

if (NOT (DEFINED NO_SOCKETS))
    check_function_exists(socket SOCKETS_FOUND)
    if (NOT SOCKETS_FOUND)
	set (NO_SOCKETS true)
    endif()
endif()
if (NO_SOCKETS) 
    set (FM_SOCKET_IO null_io.c)
elseif (WIN32)
    set (FM_SOCKET_IO server_acts.c nt_io.c)
else ()
    set (FM_SOCKET_IO server_acts.c unix_io.c)
endif()

set (FM_SRC_LIST fm_formats.c fm_dump.c lookup3.c string_conversion.c fm_get.c xml.c ${FM_SOCKET_IO})
FOREACH (FM_SRC ${FM_SRC_LIST})
    LIST (APPEND FM_MASTER_SRC_LIST fm/${FM_SRC})
ENDFOREACH()

set (COD_SRC_LIST cg.c standard.c)
FOREACH (COD_SRC ${COD_SRC_LIST})
    LIST (APPEND COD_MASTER_SRC_LIST cod/${COD_SRC})
ENDFOREACH()

set (FFS_SRC_LIST ffs.c ffs_formats.c ffs_conv.c ffs_gen.c ffs_file.c unix_io.c evol.c ffs_marshal.c )
FOREACH (FFS_SRC ${FFS_SRC_LIST})
    LIST (APPEND FFS_MASTER_SRC_LIST ffs/${FFS_SRC})
ENDFOREACH()
LIST(APPEND FFS_MASTER_SRC_LIST version.c)

find_package(BISON)
find_package(FLEX)

if (NOT BISON_FOUND OR NOT FLEX_FOUND) 
   include (BisonFlexSub )
   SETUP_BISON_FLEX_SUB ()
else()
   BISON_TARGET(CODParser cod/cod.y ${CMAKE_CURRENT_BINARY_DIR}/cod.tab.c COMPILE_FLAGS -d)
   set (HAVE_COD_PARSER_H)
   FLEX_TARGET(CODScanner cod/cod.l  ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)
   ADD_FLEX_BISON_DEPENDENCY(CODScanner CODParser)
endif ()
add_custom_command(
  OUTPUT "cod_node.c" "structs.h"
  COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/cod/struct.pl ${CMAKE_CURRENT_SOURCE_DIR}/cod/cod.structs
  DEPENDS lex.yy.c ${CMAKE_CURRENT_SOURCE_DIR}/cod/cod.structs ${CMAKE_CURRENT_SOURCE_DIR}/cod/struct.pl
)

add_custom_target(
  docs ALL
  DEPENDS "cod_node.c"
)

add_custom_target(generated
  DEPENDS cod_node.c ${BISON_CODParser_OUTPUT_SOURCE}
)

list(APPEND COD_MASTER_SRC_LIST
  ${BISON_CODParser_OUTPUT_SOURCE}
  ${CMAKE_CURRENT_BINARY_DIR}/cod_node.c
)

set(FFS_MASTER_SRC_LIST
  ${FFS_MASTER_SRC_LIST}
  ${FM_MASTER_SRC_LIST}
  ${COD_MASTER_SRC_LIST}
)

set(FFS_LIBRARY_PREFIX "" CACHE STRING
  "Prefix to prepend to the output library name")
mark_as_advanced(FFS_LIBRARY_PREFIX)

add_library(ffs ${FFS_MASTER_SRC_LIST})
set_target_properties(ffs PROPERTIES
  OUTPUT_NAME ${FFS_LIBRARY_PREFIX}ffs
  VERSION ${FFS_VERSION}
  SOVERSION ${FFS_VERSION_MAJOR})
add_library(ffs::ffs ALIAS ffs)

if(UNIX)
	target_link_libraries(ffs PRIVATE m)
endif()
if(SHARED_LIBS_SUPPORTED)
	target_link_libraries(ffs PRIVATE ${CMAKE_DL_LIBS})
endif()

target_include_directories(ffs PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/fm>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cod>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ffs>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/ffs>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

option(FFS_USE_DILL "Enable Dill code generation" ON)
if(FFS_USE_DILL)
  find_package(dill 2.3.1 REQUIRED)
  set(HAVE_DILL_H ON)
  set(FFS_CONVERSION_GENERATION_DEFAULT ${DILL_HAS_NATIVE_DCG})
  set(DO_DCG TRUE)
  target_link_libraries(ffs PRIVATE dill::dill)
endif()

option(FFS_USE_ATL "Enable the use of ATL" ON)
if(FFS_USE_ATL)
  find_package(atl 2.2.1 REQUIRED)
  set(HAVE_ATL_H ON)
  target_link_libraries(ffs PUBLIC atl::atl)
endif()

CHECK_INCLUDE_FILE(malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILE(memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILE(netdb.h HAVE_NETDB_H)
CHECK_INCLUDE_FILE(sockLib.h HAVE_SOCKLIB_H)
CHECK_INCLUDE_FILE(stdarg.h STDC_HEADERS)
CHECK_INCLUDE_FILE(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILE(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILE(sys/select.h HAVE_SYS_SELECT_H)
CHECK_INCLUDE_FILE(sys/socket.h HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILE(sys/time.h HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE(sys/times.h HAVE_SYS_TIMES_H)
CHECK_INCLUDE_FILE(sys/uio.h HAVE_SYS_UIO_H)
CHECK_INCLUDE_FILE(sys/un.h HAVE_SYS_UN_H)
CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(windows.h HAVE_WINDOWS_H)
CHECK_INCLUDE_FILE(winsock.h HAVE_WINSOCK_H)


if (${SIZEOF_SIZE_T} EQUAL ${SIZEOF_INT}) 
   set (UIO_SIZE_T_TYPE "unsigned int")
else ()
   set (UIO_SIZE_T_TYPE "unsigned long")
endif()

configure_file(ffs/ffs.h.in ffs/ffs.h @ONLY)
configure_file(CTestCustom.ctest.in CTestCustom.ctest @ONLY)

if ("${SIZEOF_LONG}" EQUAL 8) 
   set (DATA_LEN_TYPE "long")
elseif ("${SIZEOF_LONG_LONG}" EQUAL 8)
   set (DATA_LEN_TYPE "long long")
else ()
   set (DATA_LEN_TYPE "undefined")
endif()

CHECK_FUNCTION_EXISTS(strtof HAVE_STRTOF)
CHECK_FUNCTION_EXISTS(strtod HAVE_STRTOD)
CHECK_FUNCTION_EXISTS(strtold HAVE_STRTOLD)
CHECK_FUNCTION_EXISTS(getdomainname HAVE_GETDOMAINNAME)

set(CMAKE_EXTRA_INCLUDE_FILES sys/socket.h)
CHECK_TYPE_SIZE("struct iovec" IOVEC_DEFINE)
unset(CMAKE_EXTRA_INCLUDE_FILES)
CHECK_STRUCT_HAS_MEMBER("struct iovec" iov_base sys/uio.h HAS_IOV_BASE_IOVEC)

if (NOT ${HAS_IOV_BASE_IOVEC}) 
   set (NEED_IOVEC_DEFINE true)
endif()

IF (NOT DEFINED (FORMAT_SERVER_HOST) )
   set (FORMAT_SERVER_HOST "formathost.cercs.gatech.edu")
ENDIF()

IF (NOT DEFINED (FORMAT_SERVICE_DOMAIN) )
   set (FORMAT_SERVICE_DOMAIN "")
ENDIF()

install(FILES fm/fm.h cod/cod.h ${CMAKE_CURRENT_BINARY_DIR}/ffs/ffs.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(TARGETS ffs EXPORT ffs-targets
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT bin
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT shlib
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT lib
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ffs"
    COMPONENT dev)

if (CMAKE_C_COMPILER_ID MATCHES Intel) 
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -shared-intel")
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# Add all targets to the build-tree export set
export(TARGETS ffs NAMESPACE ffs::
  FILE "${PROJECT_BINARY_DIR}/ffs-targets.cmake")
 
# Create the ffs-config.cmake and ffs-config-version files
configure_file(ffs-config.cmake.in
  "${PROJECT_BINARY_DIR}/ffs-config.cmake" @ONLY)
configure_file(ffs-config-version.cmake.in
  "${PROJECT_BINARY_DIR}/ffs-config-version.cmake" @ONLY)
 
# Install the ffs-config.cmake and ffs-config-version.cmake
install(
  FILES
    "${PROJECT_BINARY_DIR}/ffs-config.cmake"
    "${PROJECT_BINARY_DIR}/ffs-config-version.cmake"
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
  COMPONENT dev
)
 
# Install the export set for use with the install-tree
install(EXPORT ffs-targets NAMESPACE ffs::
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}" COMPONENT dev
)

# display status message for important variables
option(FFS_LIBRARIES_ONLY "Whether or not to build the associated executables"
  OFF)
option(FFS_QUIET "Suppress summary output" OFF)
if(NOT FFS_QUIET)
message(STATUS)
message(STATUS "-----------------------------------------------------------------------------")
message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")
message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message(STATUS "FFS_USE_DILL = ${FFS_USE_DILL}")
message(STATUS "FFS_USE_ATL = ${FFS_USE_ATL}")
message(STATUS "FFS_LIBRARIES_ONLY = ${FFS_LIBRARIES_ONLY}")
message(STATUS "Change a value with: cmake -D<Variable>=<Value>")
message(STATUS "-----------------------------------------------------------------------------")
endif()

include(CTest)
if(BUILD_TESTING)
  enable_testing()
endif()

add_subdirectory(fm)
add_subdirectory(cod)
add_subdirectory(ffs)
