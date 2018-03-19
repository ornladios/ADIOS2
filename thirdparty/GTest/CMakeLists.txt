set(BUILD_GTEST ON CACHE INTERNAL "")
set(BUILD_GMOCK OFF CACHE INTERNAL "")
set(gtest_disable_pthreads ON)

set(CMAKE_POLICY_DEFAULT_CMP0042 NEW)
add_subdirectory(googletest EXCLUDE_FROM_ALL)

if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(gtest_main INTERFACE GTEST_LINKED_AS_SHARED_LIBRARY=1)
endif()
