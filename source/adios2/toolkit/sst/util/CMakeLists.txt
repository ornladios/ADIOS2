#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# stt_conn_tool
add_executable(sst_conn_tool ./sst_conn_tool.c)
target_link_libraries(sst_conn_tool adios2)
if(ADIOS2_HAVE_MPI)
  target_link_libraries(sst_conn_tool MPI::MPI_C)
endif()

if(WIN32)
  target_link_libraries(sst_conn_tool getopt)
endif()

target_link_libraries(sst_conn_tool sst adios2::thirdparty::EVPath)

target_include_directories(sst_conn_tool PRIVATE .. ../cp )

install(TARGETS sst_conn_tool EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
