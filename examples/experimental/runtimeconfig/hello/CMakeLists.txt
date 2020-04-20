add_executable(helloBPWriterXML helloBPWriterXML_nompi.cpp)
target_link_libraries(helloBPWriterXML adios2::cxx11)
target_compile_definitions(helloBPWriterXML PRIVATE
  -DDEFAULT_CONFIG=${CMAKE_CURRENT_SOURCE_DIR}/helloBPWriter.xml
)

if(ADIOS2_HAVE_MPI)
  add_executable(helloBPWriterXML_mpi helloBPWriterXML.cpp)
  target_link_libraries(helloBPWriterXML_mpi adios2::cxx11_mpi MPI::MPI_C)
  target_compile_definitions(helloBPWriterXML_mpi PRIVATE
    -DDEFAULT_CONFIG=${CMAKE_CURRENT_SOURCE_DIR}/helloBPWriter.xml
  )
endif()
