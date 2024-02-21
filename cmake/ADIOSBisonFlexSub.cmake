FUNCTION (SETUP_ADIOS_BISON_FLEX_SUB)

IF ((${CMAKE_SYSTEM_NAME} STREQUAL "Darwin") OR
   (${CMAKE_SYSTEM_NAME} STREQUAL "Linux"))
   set (BISON_FLEX_PRECOMPILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/toolkit/derived/parser/pregen-source")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
   set (BISON_FLEX_PRECOMPILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/toolkit/derived/parser/pregen-source/Windows")
else()
   if (NOT BISON_FOUND)
      message (FATAL_ERROR "Bison was not found and no pregenerated Bison/Flex"
              "source is available for ${CMAKE_SYSTEM_NAME}. Please install Bison or Yacc")

   else()
      message (FATAL_ERROR "Flex was not found and no pregenerated Bison/Flex" 
      	      "source is available for ${CMAKE_SYSTEM_NAME}. Please install Bison or Yacc")
   endif()
ENDIF()

ADD_CUSTOM_COMMAND(OUTPUT parser.cpp
        COMMAND ${CMAKE_COMMAND} -E copy ${BISON_FLEX_PRECOMPILE_DIR}/parser.cpp ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${BISON_FLEX_PRECOMPILE_DIR}/parser.h ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${BISON_FLEX_PRECOMPILE_DIR}/location.hh ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Using pre-generated Bison Output from ${BISON_FLEX_PRECOMPILE_DIR}")
ADD_CUSTOM_COMMAND(OUTPUT lexer.c
	COMMAND ${CMAKE_COMMAND} -E copy ${BISON_FLEX_PRECOMPILE_DIR}/lexer.cpp ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Using pre-generated Flex Output from ${BISON_FLEX_PRECOMPILE_DIR}")

set (BISON_Parser_OUTPUT_SOURCE parser.cpp PARENT_SCOPE)
ENDFUNCTION()
