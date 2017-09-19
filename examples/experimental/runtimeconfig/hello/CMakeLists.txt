if(ADIOS2_HAVE_MPI)
  add_executable(helloBPWriterXML helloBPWriterXML.cpp)
  target_include_directories(helloBPWriterXML PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(helloBPWriterXML ${MPI_C_LIBRARIES})
else()
  add_executable(helloBPWriterXML helloBPWriterXML_nompi.cpp)
endif()

target_link_libraries(helloBPWriterXML adios2)
target_compile_definitions(helloBPWriterXML PRIVATE
  -DDEFAULT_CONFIG=${CMAKE_CURRENT_SOURCE_DIR}/helloBPWriter.xml
)
