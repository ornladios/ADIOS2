cmake_minimum_required(VERSION 3.2)
project(EVPath VERSION 4.3.1 LANGUAGES C)

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# Some boilerplate to setup nice output directories
set(CMAKE_INSTALL_BINDIR bin CACHE STRING "Installation runtime subdirectory")
set(CMAKE_INSTALL_LIBDIR lib CACHE STRING "Installation library subdirectory")
set(CMAKE_INSTALL_INCLUDEDIR include
  CACHE STRING "Installation include subdirectory")
set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/EVPath
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
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Default to a RelWithDebInfo build if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE RelWithDebugInfo)
endif()

set(CPACK_DEBIAN_PACKAGE_DEPENDS "dill, atl, ffs")
set(CPACK_RPM_PACKAGE_REQUIRES "dill, atl, ffs")

include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckTypeSize)
include(CheckStructHasMember)
include(CheckCSourceRuns)
include(TestBigEndian)
include(CheckBrokenTitanCompiler)

include(CTest)
configure_file(CTestCustom.ctest.in CTestCustom.ctest @ONLY)

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
if(DEFINED EVPATH_ENABLE_PIC)
  set(EVPATH_ENABLE_PIC_DEFAULT ${EVPATH_ENABLE_PIC})
elseif(DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
  set(EVPATH_ENABLE_PIC_DEFAULT ${CMAKE_POSITION_INDEPENDENT_CODE})
else()
  set(EVPATH_ENABLE_PIC_DEFAULT ${SHARED_LIBS_SUPPORTED})
endif()
cmake_dependent_option(EVPATH_ENABLE_PIC
  "Build with Position Independent Code" ${EVPATH_ENABLE_PIC_DEFAULT}
  "SHARED_LIBS_SUPPORTED" OFF
)
mark_as_advanced(EVPATH_ENABLE_PIC)
set(CMAKE_POSITION_INDEPENDENT_CODE ${EVPATH_ENABLE_PIC})
mark_as_advanced(CMAKE_POSITION_INDEPENDENT_CODE)

if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall ")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall ")
endif()

# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
  "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if(isSystemDir EQUAL -1)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()

set(CMAKE_MACOSX_RPATH 1)

add_library(EVPath
  cm.c cm_control.c cm_formats.c cm_util.c cm_transport.c 
  cm_lock.c cm_perf.c cm_pbio.c cm_interface.c version.c
  cm_threadio.c cm_evol.c evp.c response.c metrics.c 
  dlloader.c ip_config.c chr_time.c
  revp.c evp_compat.c thin_server.c evp_threads.c ev_dfg.c)
add_library(EVPath::EVPath ALIAS EVPath)
add_library(evpath ALIAS EVPath)
target_include_directories(EVPath PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set(EVPATH_LIBRARY_PREFIX "" CACHE STRING
  "Prefix to prepend to the output library name")
mark_as_advanced(EVPATH_LIBRARY_PREFIX)
set_target_properties(EVPath PROPERTIES
  OUTPUT_NAME ${EVPATH_LIBRARY_PREFIX}evpath)

set(EVPATH_TRANSPORT_DEP_LIBS)

if(NOT DEFINED CM_SELF_FORMATS)
  set(CM_SELF_FORMATS 1)
endif()

cmake_dependent_option(EVPATH_TRANSPORT_MODULES
  "Enable stand-alone transport modules" ${BUILD_SHARED_LIBS}
  "SHARED_LIBS_SUPPORTED" OFF
)

list(INSERT CMAKE_PREFIX_PATH 0 ${CMAKE_INSTALL_PREFIX})
find_package(atl 2.2.1 REQUIRED)
find_package(ffs 1.5.1 REQUIRED)
find_package(dill 2.3.1 REQUIRED)
target_link_libraries(EVPath PUBLIC atl::atl ffs::ffs PRIVATE dill::dill)

find_package(Threads)
if(Threads_FOUND)
  target_link_libraries(EVPath PRIVATE ${CMAKE_THREAD_LIBS_INIT})
endif()

find_package(NVML)
if(NVML_FOUND)
  if(NOT BUILD_SHARED_LIBS)
    list(APPEND EXTRA_INSTALL_PACKAGES NVML)
  endif()
  target_link_libraries(EVPath PRIVATE nvml::nvml)
endif()

include(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(clock_gettime time.h HAVE_CLOCK_GETTIME)

if(NOT HAVE_CLOCK_GETTIME)
  include(CheckLibraryExists)
  CHECK_LIBRARY_EXISTS(rt clock_gettime "time.h" HAVE_CLOCK_GETTIME)
  if(HAVE_CLOCK_GETTIME)
    target_link_libraries(EVPath PRIVATE rt)
  endif()
endif()

set(CM_DEFAULT_TRANSPORT "sockets")
set(EVPATH_TRANSPORT_TARGETS)
if(EVPATH_TRANSPORT_MODULES)
  target_link_libraries(EVPath PRIVATE ${CMAKE_DL_LIBS})

  add_library(cmselect MODULE cmselect.c)
  add_library(cmsockets MODULE cmsockets.c ip_config.c)
  add_library(cmudp MODULE cmudp.c)
  add_library(cmmulticast MODULE cmmulticast.c)

  foreach(M cmselect cmsockets cmudp cmmulticast)
    set_target_properties(${M} PROPERTIES
      OUTPUT_NAME ${EVPATH_LIBRARY_PREFIX}${M})
  endforeach()

  target_link_libraries(cmselect evpath atl::atl)
  target_link_libraries(cmsockets evpath atl::atl)
  target_link_libraries(cmudp evpath atl::atl)
  target_link_libraries(cmmulticast evpath atl::atl)

  list(APPEND EVPATH_TRANSPORT_TARGETS cmselect cmsockets cmudp cmmulticast)
else()
  # dummy dyn linking in cm.c and cm_transport.c
  set(NO_DYNAMIC_LINKING 1)

  target_sources(EVPath PRIVATE cmsockets.c)
  target_sources(EVPath PRIVATE cmselect.c)
  target_sources(EVPath PRIVATE cmudp.c)
  target_sources(EVPath PRIVATE cmmulticast.c)
endif()

set(RUN_NNTI_TESTS FALSE)

# enet transport
set(RUN_ENET_TESTS FALSE)
find_package(enet 1.3.13)
if(ENET_FOUND)
  set(RUN_ENET_TESTS TRUE)
  if(EVPATH_TRANSPORT_MODULES)
    add_library(cmenet MODULE cmenet.c ip_config.c)
    set_target_properties(cmenet PROPERTIES
      OUTPUT_NAME ${EVPATH_LIBRARY_PREFIX}cmenet)

    target_link_libraries(cmenet evpath enet::enet)
    list(APPEND EVPATH_TRANSPORT_TARGETS cmenet)
  else()
    target_sources(EVPath PRIVATE cmenet.c)
    target_link_libraries(EVPath PRIVATE enet::enet)
  endif()
else()
  message(STATUS " - Enet library was not found.  This is not a fatal error, just that the Enet transport will not be built.")
endif()

# libfabric transport
find_package(LIBFABRIC)
if(LIBFABRIC_FOUND) 
  if(EVPATH_TRANSPORT_MODULES)
    add_library(cmfabric MODULE cmfabric.c ip_config.c)
    set_target_properties(cmfabric PROPERTIES
      OUTPUT_NAME ${EVPATH_LIBRARY_PREFIX}cmfabric)
    target_link_libraries(cmfabric evpath atl::atl libfabric::libfabric)
    list(APPEND EVPATH_TRANSPORT_TARGETS cmfabric)
  else()
    if(NOT BUILD_SHARED_LIBS)
      list(APPEND EXTRA_INSTALL_PACKAGES LIBFABRIC)
    endif()
    target_sources(EVPath PRIVATE cmfabric.c)
    target_link_libraries(EVPath PRIVATE libfabric::libfabric)
  endif()
else()
  message(STATUS " - LibFabric package was not found.  This is not a fatal error, just that the fabric transport will not be built.")
endif()

# ibverbs transport
set(RUN_IB_TESTS FALSE)
CHECK_LIBRARY_EXISTS(ibverbs ibv_create_qp "" HAVE_IBVERBS)
if(HAVE_IBVERBS)
  if(BUILD_TESTING)
    if(NOT CMAKE_CROSSCOMPILING)
      message(STATUS "Check MEMLOCK rlimit for IB tests")
      file(READ "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GOOD_MEMLOCK_LIMIT.c" _SOURCE)
      CHECK_C_SOURCE_RUNS("${_SOURCE}" GOOD_MEMLOCK_LIMIT)
      if(GOOD_MEMLOCK_LIMIT)
        message(STATUS "Check MEMLOCK rlimit for IB tests - yes" )
        set(RUN_IB_TESTS TRUE)
        set(RUN_NNTI_TESTS FALSE)
      else()
        message(STATUS "Check MEMLOCK rlimit for IB tests - no")
	message(STATUS "  Building InfiniBand transport, but current")
        message(STATUS "  RLIMIT_MEMLOCK value will prevent successful runs.")
        message(STATUS "  Not running IB transport tests.")
      endif()
    else()
      set(RUN_IB_TESTS TRUE)
      set(RUN_NNTI_TESTS FALSE)
    endif()
  endif()

  if(EVPATH_TRANSPORT_MODULES)
    add_library(cmib MODULE cmib.c)
    set_target_properties(cmib PROPERTIES
      OUTPUT_NAME ${EVPATH_LIBRARY_PREFIX}cmib)
    target_link_libraries(cmib evpath ibverbs)
    list(APPEND EVPATH_TRANSPORT_TARGETS cmib)
  else()
    target_sources(EVPath PRIVATE cmib.c)
    target_link_libraries(EVPath PRIVATE ibverbs)
  endif()
  set(IB_FOUND 1)
endif()

if(NOT (DEFINED CercsArch))
  execute_process(
    COMMAND cercs_arch
    OUTPUT_VARIABLE CercsArch
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  mark_as_advanced(CercsArch)
endif()

CHECK_LIBRARY_EXISTS(ugni GNI_CqCreate "" HAVE_UGNI)

if(HAVE_IBVERBS OR HAVE_UGNI)
  find_package(NNTI)
  if(NNTI_FOUND)
    if(CercsArch STREQUAL "ppc64")
      set(RUN_NNTI_TESTS FALSE)
    endif()
    if(EVPATH_TRANSPORT_MODULES)
      add_library(cmnnti MODULE cmnnti.c)
      set_target_properties(cmnnti PROPERTIES
        OUTPUT_NAME ${EVPATH_LIBRARY_PREFIX}cmnnti)
      target_link_libraries(cmnnti evpath nnti::nnti enet::enet atl::atl)
      list(APPEND EVPATH_TRANSPORT_TARGETS cmnnti)
    else()
      if(NOT BUILD_SHARED_LIBS)
        list(APPEND EXTRA_INSTALL_PACKAGES NNTI)
      endif()
      target_sources(EVPath PRIVATE cmnnti.c)
      target_link_libraries(EVPath PRIVATE nnti::nnti enet::enet atl::atl)
    endif()
  endif()
endif()

# Extra find module dependencies
foreach(P ${EXTRA_INSTALL_PACKAGES})
  configure_file(
    ${EVPath_SOURCE_DIR}/cmake/Find${P}.cmake
    ${EVPath_BINARY_DIR}/Find${P}.cmake
    COPYONLY)
  install(FILES ${EVPath_BINARY_DIR}/Find${P}.cmake
    DESTINATION ${CMAKE_INSTALL_CMAKEDIR})
endforeach()

add_custom_command(
  OUTPUT "cm_interface.c" "revp.c" "revpath.h"
  SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/evpath.h ${CMAKE_CURRENT_SOURCE_DIR}/ev_dfg.h
  COMMAND perl
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_interface.pl
    ${CMAKE_CURRENT_SOURCE_DIR}/evpath.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ev_dfg.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cm_schedule.h
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_interface.pl
    ${CMAKE_CURRENT_SOURCE_DIR}/evpath.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ev_dfg.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cm_schedule.h
)

if(Threads_FOUND AND CMAKE_USE_PTHREADS_INIT)
  set(USE_PTHREADS TRUE)
endif()

CHECK_INCLUDE_FILE(hostlib.h HAVE_HOSTLIB_H)
CHECK_INCLUDE_FILE(malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILE(memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILE(netdb.h HAVE_NETDB_H)
CHECK_INCLUDE_FILE(sockLib.h HAVE_SOCKLIB_H)
CHECK_INCLUDE_FILE(stdarg.h STDC_HEADERS)
CHECK_INCLUDE_FILE(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILE(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILE(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILE(sys/select.h HAVE_SYS_SELECT_H)
CHECK_INCLUDE_FILE(sys/socket.h HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILE(sys/sockio.h HAVE_SYS_SOCKIO_H)
CHECK_INCLUDE_FILE(sys/time.h HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE(sys/times.h HAVE_SYS_TIMES_H)
CHECK_INCLUDE_FILE(sys/uio.h HAVE_SYS_UIO_H)
CHECK_INCLUDE_FILE(sys/un.h HAVE_SYS_UN_H)
CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(windows.h HAVE_WINDOWS_H)
CHECK_INCLUDE_FILE(winsock.h HAVE_WINSOCK_H)

CHECK_STRUCT_HAS_MEMBER("struct fd_set" "fds_bits" "sys/select.h" HAVE_FDS_BITS)

CHECK_TYPE_SIZE("int"   SIZEOF_INT)
CHECK_TYPE_SIZE("long"   SIZEOF_LONG)
TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

CHECK_FUNCTION_EXISTS(writev HAVE_WRITEV)
CHECK_FUNCTION_EXISTS(uname HAVE_UNAME)
CHECK_FUNCTION_EXISTS(getdomainname HAVE_GETDOMAINNAME)
CHECK_FUNCTION_EXISTS(getloadavg HAVE_GETLOADAVG)
CHECK_FUNCTION_EXISTS(gettimeofday HAVE_GETTIMEOFDAY)
CHECK_FUNCTION_EXISTS(getifaddrs HAVE_GETIFADDRS)

try_compile(HAVE_MAC_SYSCTL
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_mac_sysctl.c
)
try_compile(HAVE_SYSINFO
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_sysinfo.c
)
try_compile(HAVE_SYSCONF
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_sysconf.c
)

if(CMAKE_C_COMPILER_ID MATCHES "Intel") 
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -shared-intel")
endif()

install(
  FILES
    evpath.h
    cm_schedule.h
    ev_dfg.h 
    cm_transport.h
    ${CMAKE_CURRENT_BINARY_DIR}/revpath.h 
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(TARGETS EVPath ${EVPATH_TRANSPORT_TARGETS} EXPORT EVPathTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

set(EVPATH_LIBRARY_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(EVPATH_LIBRARY_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib)

add_executable(cmprobe cmprobe.c)
target_link_libraries(cmprobe evpath atl::atl)

if(CMAKE_C_COMPILER_ID MATCHES "Intel") 
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -shared-intel")
endif()

find_program(SSH ssh)
if(SSH) 
  set(SSH_PATH "${SSH}")
endif() 

if(BUILD_TESTING) 
  enable_testing()

  add_subdirectory(tests)
  add_subdirectory(rtests)
  add_subdirectory(mtests)
  add_subdirectory(dfg_tests)

  find_package(MPI)
  if(MPI_FOUND)
    add_subdirectory(mpi_tests)
  endif()

  add_subdirectory(examples)
endif()

export(EXPORT EVPathTargets NAMESPACE EVPath::
  FILE "${PROJECT_BINARY_DIR}/EVPathTargets.cmake")

# This "exports" all targets which have been put into the export set
# "EVPathTargets".  This means that cmake generates a file with the given
# filename, which can later on be loaded by projects using this package.  This
# file contains add_library(EVPath IMPORTED) statements for each target in the
# export set, so when loaded later on cmake will create "imported" library
# targets from these, which can be used in many ways in the same way as a
# normal library target created via a normal add_library().
install(EXPORT EVPathTargets NAMESPACE EVPath::
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR})

# Create a EVPathConfig.cmake file. <name>Config.cmake files are searched by
# find_package() automatically. We configure that file so that we can put any
# information we want in it, e.g. version numbers, include directories, etc.
configure_file(
  EVPathConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/EVPathConfig.cmake
  @ONLY
)

# So, this big ol thing...
# This function will walk the dependency graph of a target to determine the full
# list of transitive library and header information needed to use it.  The
# purpose is to extract the necessary usage requirements for a non-CMake
# project to consume evpath
function(traverse_target_interface in_target out_libs out_inc_dirs)
  get_target_property(libs ${in_target} INTERFACE_LINK_LIBRARIES)
  if(NOT libs)
    set(libs)
  endif()
  get_target_property(inc_dirs ${in_target} INTERFACE_INCLUDE_DIRECTORIES)
  if(NOT inc_dirs)
    set(inc_dirs)
  endif()
  list(INSERT libs 0 "$<TARGET_FILE:${in_target}>")

  set(_has_targets TRUE)
  while(_has_targets)
    set(_libs)
    set(_has_targets FALSE)
    foreach(L IN LISTS libs)
      if(L MATCHES "<LINK_ONLY:(.*)>")
        set(L ${CMAKE_MATCH_1})
      endif()
      if(TARGET ${L})
        set(_has_targets TRUE)
        get_target_property(L_libs ${L} INTERFACE_LINK_LIBRARIES)
        get_target_property(L_inc_dirs ${L} INTERFACE_INCLUDE_DIRECTORIES)
        get_target_property(L_type ${L} TYPE)
        if(L_type MATCHES "(STATIC|SHARED)_LIBRARY")
          list(APPEND _libs "$<TARGET_FILE:${L}>")
        endif()
        if(L_libs)
          list(APPEND _libs ${L_libs})
        endif()
        if(L_inc_dirs)
          list(APPEND inc_dirs ${L_inc_dirs})
        endif()
      elseif(L MATCHES "^(/|-|\\$<)")
        list(APPEND _libs "${L}")
      else()
        list(APPEND _libs "-l${L}")
      endif()
    endforeach()
    set(libs ${_libs})
  endwhile()

  set(${out_libs} ${libs} PARENT_SCOPE)
  set(${out_inc_dirs} ${inc_dirs} PARENT_SCOPE)
endfunction()

traverse_target_interface(EVPath
  EVPATH_INTERFACE_LINK_INFO
  EVPATH_INTERFACE_INCLUDE_DIRECTORIES)

# Filter out any build interface items
set(_EVPATH_INSTALL_INC_DIRS)
foreach(D IN LISTS EVPATH_INTERFACE_INCLUDE_DIRECTORIES)
  if(D MATCHES "<BUILD_INTERFACE:(.*)>")
    continue()
  elseif(D MATCHES "<INSTALL_INTERFACE:(.*)>")
    list(APPEND _EVPATH_INSTALL_INC_DIRS
      "${CMAKE_INSTALL_PREFIX}/${CMAKE_MATCH_1}")
  else()
    list(APPEND _EVPATH_INSTALL_INC_DIRS "${D}")
  endif()
endforeach()
set(EVPATH_INTERFACE_INCLUDE_DIRECTORIES ${_EVPATH_INSTALL_INC_DIRS})

# Create a evpath_config
list(REMOVE_DUPLICATES EVPATH_INTERFACE_LINK_INFO)
list(REMOVE_DUPLICATES EVPATH_INTERFACE_INCLUDE_DIRECTORIES)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/evpath_config.in
  ${CMAKE_CURRENT_BINARY_DIR}/evpath_config.gen
  @ONLY
)
file(GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/evpath_config
  INPUT ${CMAKE_CURRENT_BINARY_DIR}/evpath_config.gen
)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/evpath_config
  DESTINATION ${CMAKE_INSTALL_BINDIR})

# Additionally, when cmake has found a EVPathConfig.cmake, it can check for a
# EVPathConfigVersion.cmake in the same directory when figuring out the version
# of the package when a version has been specified in the find_package() call,
# e.g. find_package(EVPath 1.0)
configure_file(
  EVPathConfigVersion.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/EVPathConfigVersion.cmake
  @ONLY
)

# Install these two files into the same directory as the generated exports-file.
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/EVPathConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/EVPathConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

if(TEST_INSTALL_DIRECTORY) 
  set(EVPATH_TEST_INSTALL_DIR ${TEST_INSTALL_DIRECTORY})
endif()

# display status message for important variables
option(EVPATH_QUIET "Suppress summary output" OFF)
if(NOT EVPATH_QUIET)
  message(STATUS)
  message(STATUS "-----------------------------------------------------------------------------" )
  message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}" )
  message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
  message(STATUS "BUILD_TESTING = ${BUILD_TESTING}  (options are: ON, OFF)")
  message(STATUS "BUILD_SHARED_LIBS = ${BUILD_SHARED_LIBS}  (options are: ON, OFF, default OFF)")
  message(STATUS "EVPATH_TRANSPORT_MODULES = ${EVPATH_TRANSPORT_MODULES} (${EVPATH_TRANSPORT_TARGETS})")
  message(STATUS "Change a value with: cmake -D<Variable>=<Value>" )
  message(STATUS "-----------------------------------------------------------------------------" )
endif()
