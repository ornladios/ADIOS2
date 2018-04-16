cmake_minimum_required(VERSION 2.8)

if (NATIVE_CG)
set (TESTS regress ctest call-test t1 pkg_test)

INCLUDE_DIRECTORIES(BEFORE ${dill_BINARY_DIR} ${dill_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

ADD_CUSTOM_COMMAND(
   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/regress.c
   COMMAND perl -w ${CMAKE_CURRENT_SOURCE_DIR}/test-gen ${TEST_PERL_FLAGS} > regress.c
   MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/test-gen
)

ADD_CUSTOM_COMMAND(
   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/call-test.c
   COMMAND perl -w ${CMAKE_CURRENT_SOURCE_DIR}/call-gen ${TEST_PERL_FLAGS} > call-test.c
   MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/call-gen 
)
foreach (TEST ${TESTS} )
    ADD_EXECUTABLE(dill_${TEST} ${TEST}.c)
    TARGET_LINK_LIBRARIES(dill_${TEST} dill ${DIS_LIBS})
    ADD_TEST(NAME dill_${TEST} COMMAND dill_${TEST} ${TEST_PERL_FLAGS})
endforeach()

TARGET_LINK_LIBRARIES(dill_regress m)
SET_TARGET_PROPERTIES(dill_regress PROPERTIES COMPILE_FLAGS "-O0")
ADD_EXECUTABLE(cplus cplus.cc)
TARGET_LINK_LIBRARIES(cplus dill ${DIS_LIBS})
ADD_TEST(NAME dill_cplus COMMAND cplus)
ADD_EXECUTABLE(stest stest.c)
TARGET_LINK_LIBRARIES(stest dill ${DIS_LIBS})
ADD_TEST(NAME dill_stest COMMAND stest)

set_tests_properties (dill_call-test
  PROPERTIES PASS_REGULAR_EXPRESSION "No errors!")
set_tests_properties (dill_regress
  PROPERTIES PASS_REGULAR_EXPRESSION "No errors!")
set_tests_properties (dill_stest
  PROPERTIES PASS_REGULAR_EXPRESSION "hello world!
success!
hello world!
success!
hello world!
success!")
set_tests_properties (dill_ctest
  PROPERTIES PASS_REGULAR_EXPRESSION "Hello: 10 20 30 40
Hello: 10 20 30 40
Hello: 10 20 30 40 50 60 70 80 90 100
Hello: 1.000000e[+]01 2.000000e[+]01 3.000000e[+]01 4.000000e[+]01 5.000000e[+]01 6.000000e[+]01 7.000000e[+]01 8.000000e[+]01 9.000000e[+]01 1.000000e[+]02")
SET_TARGET_PROPERTIES(dill_call-test PROPERTIES LINKER_LANGUAGE C)
SET_TARGET_PROPERTIES(dill_ctest PROPERTIES LINKER_LANGUAGE C)
SET_TARGET_PROPERTIES(dill_pkg_test PROPERTIES LINKER_LANGUAGE C)
endif (NATIVE_CG)


