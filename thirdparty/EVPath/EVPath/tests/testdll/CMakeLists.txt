add_library(foo MODULE foo.c)
# Always use "lib" prefix since filter_test expects "libfoo.la" -> "libfoo.dll"
# WINDOWS_EXPORT_ALL_SYMBOLS ensures filter/transform functions are exported
set_target_properties(foo PROPERTIES
  PREFIX ""
  OUTPUT_NAME libfoo
  WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

# For multi-config generators (MSVC), output to a fixed location
# to avoid the Release/Debug subdirectory issue
set_target_properties(foo PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}
  LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}
  LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}
)
if (TEST_INSTALL_DIRECTORY) 
  install(TARGETS foo DESTINATION "${TEST_INSTALL_DIRECTORY}/testdll")
endif() 
