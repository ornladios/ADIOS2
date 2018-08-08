#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)

    add_executable(hello_datamanSubscribeWriter helloDataManSubscribeWriter.cpp)
	target_link_libraries(hello_datamanSubscribeWriter adios2 MPI::MPI_C)

	add_executable(hello_datamanP2PWriter helloDataManP2PWriter.cpp)
	target_link_libraries(hello_datamanP2PWriter adios2 MPI::MPI_C)

endif()
