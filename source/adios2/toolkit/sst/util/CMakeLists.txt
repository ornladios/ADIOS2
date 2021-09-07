#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  set(maybe_adios2_core_mpi adios2_core_mpi)
else()
  set(maybe_adios2_core_mpi)
endif()

# stt_conn_tool
add_executable(sst_conn_tool sst_conn_tool.c sst_conn_tool.cxx)
target_link_libraries(sst_conn_tool ${maybe_adios2_core_mpi} adios2_core)
set_property(TARGET sst_conn_tool PROPERTY OUTPUT_NAME sst_conn_tool${ADIOS2_EXECUTABLE_SUFFIX})

if(WIN32)
  target_link_libraries(sst_conn_tool getopt)
endif()

target_link_libraries(sst_conn_tool sst adios2::thirdparty::EVPath)

target_include_directories(sst_conn_tool PRIVATE .. ../cp )

install(TARGETS sst_conn_tool EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT adios2_tools-runtime
)

# libfabric_mpi_compatibility
if(ADIOS2_HAVE_MPI AND ADIOS2_SST_HAVE_LIBFABRIC)
  add_executable(libfabric_mpi_compatibility libfabric_mpi_compatibility.cpp)
  target_link_libraries(libfabric_mpi_compatibility adios2_core_mpi adios2_core sst adios2::thirdparty::EVPath)
  target_include_directories(libfabric_mpi_compatibility PRIVATE .. ../cp ../dp )
  set_property(TARGET libfabric_mpi_compatibility PROPERTY OUTPUT_NAME libfabric_mpi_compatibility${ADIOS2_EXECUTABLE_SUFFIX})

  if(WIN32)
    target_link_libraries(libfabric_mpi_compatibility getopt)
  endif()

  install(TARGETS libfabric_mpi_compatibility EXPORT adios2
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT adios2_tools-runtime
  )
endif()
