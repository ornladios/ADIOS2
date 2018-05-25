message_start_thirdparty()

set(BUILD_GTEST ON CACHE INTERNAL "")
set(BUILD_GMOCK OFF CACHE INTERNAL "")
if(BUILD_SHARED_LIBS)
  set(gtest_force_shared_crt ON CACHE INTERNAL "")
endif()
set(BUILD_SHARED_LIBS OFF)

set(CMAKE_POLICY_DEFAULT_CMP0042 NEW)

add_subdirectory(googletest EXCLUDE_FROM_ALL)

add_library(gtest_interface INTERFACE)
target_link_libraries(gtest_interface INTERFACE gtest)
if(BUILD_SHARED_LIBS)
  target_compile_definitions(gtest_interface INTERFACE
    GTEST_LINKED_AS_SHARED_LIBRARY=1
  )
endif()

message_end_thirdparty()
