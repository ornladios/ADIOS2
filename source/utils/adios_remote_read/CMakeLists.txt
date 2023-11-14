#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}
        DESTINATION ${PROJECT_BINARY_DIR}
)


add_executable(adios_remote_read adios_remote_read.cpp)
target_link_libraries(adios_remote_read adios2::cxx11_mpi MPI::MPI_C adios2_core_mpi)
if(WIN32)
    target_link_libraries(adios_remote_read getopt)
endif()
set_property(TARGET adios_remote_read PROPERTY OUTPUT_NAME adios2_remote_read${ADIOS2_EXECUTABLE_SUFFIX})


install(TARGETS adios_remote_read EXPORT adios2
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT adios2_tools-runtime
)
