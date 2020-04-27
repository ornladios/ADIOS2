#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/verinfo.h.in
  ${PROJECT_BINARY_DIR}/verinfo.h
  @ONLY
)

# BPLS
add_executable(bpls ./bpls/bpls.cpp)
target_link_libraries(bpls adios2_core adios2sys adios2::thirdparty::pugixml)
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
#target_link_libraries(bpsplit adios2::cxx11 adios2sys_interface adios2::thirdparty::pugixml)
#install(TARGETS bpsplit EXPORT adios2
#  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#)

# ADIOS_REORGANIZE
set(adios_reorganize_srcs
  adios_reorganize/main.cpp
  adios_reorganize/Reorganize.cpp
  Utils.cpp
  )
if(ADIOS2_HAVE_MGARD)
  set(maybe_mgard MGARD::MGARD)
else()
  set(maybe_mgard)
endif()
add_executable(adios_reorganize ${adios_reorganize_srcs})
target_link_libraries(adios_reorganize PRIVATE adios2_core ${maybe_mgard})

if(ADIOS2_HAVE_MPI)
  add_executable(adios_reorganize_mpi ${adios_reorganize_srcs})
  target_link_libraries(adios_reorganize_mpi PRIVATE adios2_core_mpi ${maybe_mgard})
  set(maybe_adios_reorganize_mpi adios_reorganize_mpi)
else()
  set(maybe_adios_reorganize_mpi)
endif()

install(TARGETS adios_reorganize
  ${maybe_adios_reorganize_mpi}
  EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(ADIOS2_HAVE_MPI)
  add_subdirectory(adios_iotest)
endif()

if(Python_Interpreter_FOUND)
  add_subdirectory(bp4dbg)
endif()

install(PROGRAMS adios_deactivate_bp.sh DESTINATION ${CMAKE_INSTALL_BINDIR})
