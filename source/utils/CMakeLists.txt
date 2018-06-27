#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# BPLS
add_executable(bpls ./bpls/bpls.cpp)
target_link_libraries(bpls adios2 adios2sys_interface)
install(TARGETS bpls EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
file(GENERATE
  OUTPUT ${PROJECT_BINARY_DIR}/BPLS.cmake
  INPUT ${PROJECT_SOURCE_DIR}/cmake/BPLS.cmake.gen
)

# ADIOS2_REORGANIZE
add_executable(adios2_reorganize 
               ./adios2_reorganize/main.cpp 
               ./adios2_reorganize/Reorganize.cpp 
               Utils.cpp)
target_link_libraries(adios2_reorganize adios2)
install(TARGETS adios2_reorganize EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Add MPI dependency to parallel tools
if(ADIOS2_HAVE_MPI)
  target_link_libraries(adios2_reorganize MPI::MPI_C)
endif()



