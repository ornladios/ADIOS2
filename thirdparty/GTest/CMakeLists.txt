set(BUILD_GTEST ON)
set(BUILD_GMOCK OFF)

add_subdirectory(googletest)

# Fix missing usage requirements
target_include_directories(gtest
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest/include
)
