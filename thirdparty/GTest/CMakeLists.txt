set(BUILD_GTEST ON)
set(BUILD_GMOCK OFF)
set(gtest_disable_pthreads ON)

add_subdirectory(googletest EXCLUDE_FROM_ALL)
