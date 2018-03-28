#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
add_library(sst
  dp/dp.c
  dp/evpath_dp.c
  cp/cp_reader.c
  cp/cp_writer.c
  cp/cp_common.c
  cp/ffs_marshal.c
)
if(ADIOS2_SST_HAVE_LIBFABRIC)
  target_sources(sst PRIVATE dp/rdma_dp.c)
  target_link_libraries(sst PRIVATE libfabric::libfabric)
endif()

# Set library version information
set_target_properties(sst PROPERTIES
  OUTPUT_NAME adios2_sst
  VERSION ${ADIOS2_VERSION}
  SOVERSION ${ADIOS2_VERSION_MAJOR}
)

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

# Ideally we'd like to use the c_std_99 meta feature, but that wasn't available
# until CMake 3.8.  However, requiring the c_restrict feature effectively
# achieves the same result since it's a C99 feature
target_compile_features(sst PRIVATE c_restrict)

target_include_directories(sst PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(sst PRIVATE EVPath::EVPath)
target_link_libraries(sst PUBLIC MPI::MPI_C)

install(TARGETS sst EXPORT adios2Exports
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
