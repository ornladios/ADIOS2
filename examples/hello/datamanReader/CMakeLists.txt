#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)

	add_executable(DataManCallbackReceiver DataManCallbackReceiver.cpp)
	target_link_libraries(DataManCallbackReceiver adios2 MPI::MPI_C)

    add_executable(hello_datamanSubscribeReader helloDataManSubscribeReader.cpp)
	target_link_libraries(hello_datamanSubscribeReader adios2 MPI::MPI_C)

    add_executable(hello_datamanP2PReader helloDataManP2PReader.cpp)
	target_link_libraries(hello_datamanP2PReader adios2 MPI::MPI_C)

endif()

