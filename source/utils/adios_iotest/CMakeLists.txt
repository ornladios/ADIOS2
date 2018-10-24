#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/iotest-config
  DESTINATION ${PROJECT_BINARY_DIR}
)

add_executable(adios_iotest settings.cpp decomp.cpp processConfig.cpp adios_iotest.cpp)
target_link_libraries(adios_iotest adios2 MPI::MPI_C)

install(TARGETS adios_iotest EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY iotest-config/ 
   DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/iotest-config
)
