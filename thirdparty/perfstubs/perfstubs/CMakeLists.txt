if (ADIOS2_HAVE_Profiling)
  set(PERFSTUBS_USE_TIMERS ON CACHE INTERNAL "")
else()
  set(PERFSTUBS_USE_TIMERS OFF CACHE INTERNAL "")
endif()

if (NOT BUILD_SHARED_LIBS)
  set(PERFSTUBS_USE_TIMERS OFF CACHE INTERNAL "")
endif()

set(PS_LIBRARY_PREFIX adios2${ADIOS2_LIBRARY_SUFFIX}_ CACHE INTERNAL "")

set(PerfStubs_VERSION_MAJOR 0)
set(PerfStubs_VERSION_MINOR 1)
set(PerfStubs_VERSION ${PerfStubs_VERSION_MAJOR}.${PerfStubs_VERSION_MINOR})

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

set(ps_headers
  perfstubs_api/timer.h
  perfstubs_api/tool.h
)

set(ps_src
  perfstubs_api/timer.c
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/perfstubs_api/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/perfstubs_api/config.h"
)

add_library(perfstubs ${ps_src})

set_target_properties(perfstubs PROPERTIES
  OUTPUT_NAME ${PS_LIBRARY_PREFIX}perfstubs
  VERSION ${PerfStubs_VERSION}
  SOVERSION ${PerfStubs_VERSION_MAJOR})

target_link_libraries(perfstubs INTERFACE dl m)

target_include_directories(perfstubs PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/thirdparty>
)
