
ExternalProject_Add(googletest
  URL     https://github.com/google/googletest/archive/release-1.8.0.zip
  URL_MD5 adfafc8512ab65fd3cf7955ef0100ff5
  ${EP_ARGS}
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/install
    -DCMAKE_POLICY_DEFAULT_CMP0048=OLD
    -DCMAKE_POLICY_DEFAULT_CMP0063=NEW
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
    -DBUILD_GMOCK=OFF
    -DBUILD_GTEST=ON
    -Dgtest_build_samples=OFF
    -Dgtest_build_tests=OFF
    -Dgtest_hide_internal_symbols=ON
    -Dgtest_disable_pthreads=OFF
    -Dgtest_force_shared_crt=OFF)

function(get_libfile var name)
  if(BUILD_SHARED_LIBS)
    set(${var} ${CMAKE_SHARED_LIBRARY_PREFIX}${name}${CMAKE_SHARED_LIBRARY_SUFFIX} PARENT_SCOPE)
  else()
    set(${var} ${CMAKE_STATIC_LIBRARY_PREFIX}${name}${CMAKE_STATIC_LIBRARY_SUFFIX} PARENT_SCOPE)
  endif()
endfunction()

get_libfile(GTEST_LIBRARY gtest)
get_libfile(GTEST_MAIN_LIBRARY gtest_main)
set(GTEST_ROOT ${CMAKE_CURRENT_BINARY_DIR}/install)
set(GTEST_INCLUDE_DIR ${GTEST_ROOT}/include
  CACHE PATH "")
set(GTEST_LIBRARY ${GTEST_ROOT}/lib/${GTEST_LIBRARY}
  CACHE FILEPATH "")
set(GTEST_MAIN_LIBRARY ${GTEST_ROOT}/lib/${GTEST_MAIN_LIBRARY}
  CACHE FILEPATH "")
