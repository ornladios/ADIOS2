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

if(ADIOS2_HAVE_ZFP)
  target_sources(sst PRIVATE cp/ffs_zfp.c)
  target_link_libraries(sst PRIVATE zfp::zfp)
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

if (NOT MSVC)
  target_compile_features(sst PRIVATE ${ADIOS2_C99_FEATURES})
endif()

target_include_directories(sst PRIVATE
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/source>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(sst PRIVATE adios2::thirdparty::EVPath)
if(ADIOS2_HAVE_MPI)
  target_link_libraries(sst PUBLIC MPI::MPI_C)
endif()

install(TARGETS sst EXPORT adios2Exports
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
