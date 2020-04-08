#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/verinfo.h.in
  ${PROJECT_BINARY_DIR}/verinfo.h
  @ONLY
)

if(ADIOS2_HAVE_MPI)
  set(maybe_adios2_core_mpi adios2_core_mpi)
else()
  set(maybe_adios2_core_mpi)
endif()

# BPLS
add_executable(bpls ./bpls/bpls.cpp)
target_link_libraries(bpls adios2_core adios2sys pugixml)
target_include_directories(bpls PRIVATE ${PROJECT_BINARY_DIR})
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
target_link_libraries(adios_reorganize PRIVATE adios2_cxx11 ${maybe_adios2_core_mpi} adios2_core)
install(TARGETS adios_reorganize EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(ADIOS2_HAVE_MGARD)
  target_link_libraries(adios_reorganize MGARD::MGARD)
endif()

if(ADIOS2_HAVE_MPI)
  add_subdirectory(adios_iotest)
endif()

if(Python_Interpreter_FOUND)
  add_subdirectory(bp4dbg)
endif()

install(PROGRAMS adios_deactivate_bp.sh DESTINATION ${CMAKE_INSTALL_BINDIR})
