add_library(sst
  dp/dp.c
  dp/evpath_dp.c
  cp/cp_reader.c
  cp/cp_writer.c
  cp/cp_common.c
  cp/ffs_marshal.c
)

if(SST_HAVE_LIBFABRIC)
    target_sources(sst PRIVATE dp/rdma_dp.c)
endif()

#------------------------------------------------------------------------------#
# Generate SST defines
#------------------------------------------------------------------------------#
set(SST_CONFIG_OPTS
  LIBFABRIC
)
include(SSTFunctions)
GenerateSSTHeaderConfig(${SST_CONFIG_OPTS})
install(FILES ${PROJECT_BINARY_DIR}/source/adios2/toolkit/sst/SSTConfig.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2/toolkit/sst
)

target_compile_features(sst PRIVATE c_std_99)
target_include_directories(sst PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<BUILD_INTERFACE:${ADIOS2_SOURCE_DIR}/source/adios2/toolkit/sst>
    $<BUILD_INTERFACE:${ADIOS2_BINARY_DIR}/source/adios2/toolkit/sst>
)
target_link_libraries(sst PRIVATE EVPath::EVPath)
target_link_libraries(sst PUBLIC MPI::MPI_C)

if(SST_HAVE_LIBFABRIC)
    target_link_libraries(sst PRIVATE ${LIBFABRIC_LIBRARY})
endif()

install(TARGETS sst EXPORT adios2Exports
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
