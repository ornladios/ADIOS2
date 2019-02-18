if(ADIOS2_HAVE_Fortran)
  set(ADIOS2_CONFIG_FORTRAN 1)
else()
  set(ADIOS2_CONFIG_FORTRAN 0)
endif()
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/generate-adios2-config.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/generate-adios2-config.sh
  @ONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/adios2-config.pre.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/adios2-config.pre.sh
  @ONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/adios2-config.post.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/adios2-config.post.sh
  @ONLY
)
install(CODE "
  execute_process(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/generate-adios2-config.sh)
"
)
