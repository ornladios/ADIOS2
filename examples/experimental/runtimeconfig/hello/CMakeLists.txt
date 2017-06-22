if(ADIOS2_HAVE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)

  add_executable(helloBPWriterXML helloBPWriterXML.cpp)
  target_include_directories(helloBPWriterXML PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(helloBPWriterXML ${MPI_C_LIBRARIES})
else()
  add_executable(helloBPWriterXML helloBPWriterXML_nompi.cpp)
endif()

target_link_libraries(helloBPWriterXML adios2)
