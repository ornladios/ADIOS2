add_library(sst
  dp/dp.c
  dp/evpath_dp.c
  cp/cp_reader.c
  cp/cp_writer.c
  cp/cp_common.c
  cp/ffs_marshal.c
)

# Ideally we'd like to use the c_std_99 meta feature, but that wasn't available
# until CMake 3.8.  However, requiring the c_restrict feature effectively
# achieves the same result since it's a C99 feature
target_compile_features(sst PRIVATE c_restrict)

target_include_directories(sst PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sst PRIVATE EVPath::EVPath)
target_link_libraries(sst PUBLIC MPI::MPI_C)

install(TARGETS sst EXPORT adios2Exports
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
