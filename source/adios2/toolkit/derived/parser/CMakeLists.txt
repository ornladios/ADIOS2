project(simple-test)
cmake_minimum_required(VERSION 3.18)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 11)

set(PARSER_SOURCE "./" CACHE PATH "Expression Parser source code")

# Hardcoded for debugging - DELETE
set(BISON_EXECUTABLE "/usr/local/Cellar/bison/3.8.2/bin/bison" CACHE PATH "Bison executable")
set(FLEX_EXECUTABLE "/usr/local/Cellar/flex/2.6.4_2/bin/flex" CACHE PATH "Flex executable")

find_package(BISON)
find_package(FLEX)

# if(NOT ADIOS2_EXPRESSIONS_REGEN OR NOT BISON_FOUND OR NOT FLEX_FOUND)
#
# else()
  BISON_TARGET(MyParser
    parser.y
    ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h)
  FLEX_TARGET(MyScanner
    lexer.l
    ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/lexer.h)
  ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)
# endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${PARSER_SOURCE})
add_executable(simple-test 
  ${PARSER_SOURCE}/driver.cpp
  ${PARSER_SOURCE}/ASTNode.cpp
  ${PARSER_SOURCE}/ASTNode.h
  ${BISON_MyParser_OUTPUTS}
  ${FLEX_MyScanner_OUTPUTS}
)

target_include_directories(simple-test PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${PARSER_SOURCE})
