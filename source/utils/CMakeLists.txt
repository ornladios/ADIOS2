#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# BPLS
add_executable(bpls ./bpls/bpls.cpp)
target_link_libraries(bpls adios2 adios2sys_interface pugixml)
install(TARGETS bpls EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/bpls/bpls.cmake.gen.in
  ${PROJECT_BINARY_DIR}/bpls.cmake.gen
  @ONLY
)
file(GENERATE
  OUTPUT ${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake
  INPUT ${PROJECT_BINARY_DIR}/bpls.cmake.gen
)


# BPSPLIT
#add_executable(bpsplit ./bpsplit/bpsplit.cpp)
#target_link_libraries(bpsplit adios2 adios2sys_interface pugixml)
#install(TARGETS bpsplit EXPORT adios2
#  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#)

# ADIOS_REORGANIZE
add_executable(adios_reorganize 
               ./adios_reorganize/main.cpp 
               ./adios_reorganize/Reorganize.cpp 
               Utils.cpp)
target_link_libraries(adios_reorganize adios2)
install(TARGETS adios_reorganize EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Add MPI dependency to parallel tools
if(ADIOS2_HAVE_MPI)
  target_link_libraries(adios_reorganize MPI::MPI_C)
endif()

if(ADIOS2_HAVE_MGARD)
  target_link_libraries(adios_reorganize MGARD::MGARD)
endif()

if(ADIOS2_HAVE_MPI)
  add_subdirectory(adios_iotest)
endif()

# bp4dbg Python tool
install(PROGRAMS bp4dbg/bp4dbg.py bp4dbg/bp4dbg_data.py bp4dbg/bp4dbg_utils.py 
        bp4dbg/bp4dbg_metadata.py bp4dbg/bp4dbg_idxtable.py
        DESTINATION ${CMAKE_INSTALL_BINDIR}/bp4dbg)

install(PROGRAMS adios_deactivate_bp.sh DESTINATION ${CMAKE_INSTALL_BINDIR})
