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
  mpidummy.cpp
  mpiwrap.cpp
)

target_link_libraries(sst PRIVATE taustubs)

if(ADIOS2_SST_HAVE_LIBFABRIC)
  target_sources(sst PRIVATE dp/rdma_dp.c)
  target_link_libraries(sst PRIVATE libfabric::libfabric)
  set(CMAKE_REQUIRED_INCLUDES ${LIBFABRIC_INCLUDE_DIRS})
  CHECK_INCLUDE_FILE(rdma/fi_ext_gni.h ADIOS2_SST_HAVE_FI_GNI)
  if(ADIOS2_SST_HAVE_CRAY_DRC)
    target_link_libraries(sst PRIVATE craydrc::craydrc)
  endif()
endif()

if(ADIOS2_SST_HAVE_NVSTREAM)
  target_sources(sst PRIVATE dp/nvstream_dp.c dp/nvswrapper.cpp)
  target_link_libraries(sst PRIVATE NVStream::NVStream ${Boost_LIBRARIES})
  set(CMAKE_REQUIRED_INCLUDES ${NVSTREAM_INCLUDE_DIRS})
endif()

if(ADIOS2_HAVE_ZFP)
  target_sources(sst PRIVATE cp/ffs_zfp.c)
  target_link_libraries(sst PRIVATE zfp::zfp)
endif()

# Set library version information
set_target_properties(sst PROPERTIES
  OUTPUT_NAME adios2_sst
  VERSION ${ADIOS2_LIBRARY_VERSION}
  SOVERSION ${ADIOS2_VERSION_MAJOR}
)

#------------------------------------------------------------------------------#
# Generate SST defines
#------------------------------------------------------------------------------#
set(SST_CONFIG_OPTS
  LIBFABRIC
  FI_GNI
  CRAY_DRC
  NVSTREAM
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
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>
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

add_subdirectory(util)
