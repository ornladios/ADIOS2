if (HAVE_COD_H)
   set(TESTS evtest extract_test remote_terminal_test)
else()
   set(TESTS)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR}/test_common)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/revtests.tsts "" )
foreach(T ${TESTS})
  add_executable(revp_${T} ${T}.c ${CMAKE_SOURCE_DIR}/test_common/support.c)
  target_link_libraries(revp_${T} evpath atl::atl)
  add_test(NAME revp_${T} COMMAND "revp_${T}")
  list(APPEND REVTESTS "revp_${T}")
  file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/revtests.tsts "revp_${T}\n" )
endforeach()

if(TEST_INSTALL_DIRECTORY) 
  install(TARGETS ${REVTESTS} DESTINATION "${TEST_INSTALL_DIRECTORY}")
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/revtests.tsts
    DESTINATION "${TEST_INSTALL_DIRECTORY}")
endif()
