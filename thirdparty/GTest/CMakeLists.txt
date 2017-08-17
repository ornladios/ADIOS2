set(BUILD_GTEST ON)
set(BUILD_GMOCK OFF)
set(gtest_disable_pthreads ON)

set(CMAKE_POLICY_DEFAULT_CMP0042 NEW)
add_subdirectory(googletest EXCLUDE_FROM_ALL)
