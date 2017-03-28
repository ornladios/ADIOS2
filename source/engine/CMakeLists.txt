target_sources(adios2 PRIVATE
  engine/bp/BPFileReader.cpp
  engine/bp/BPFileWriter.cpp
)

if(ADIOS_USE_ADIOS1)
  find_package(ADIOS REQUIRED)
  target_sources(adios2 PRIVATE
    adios1/ADIOS1Reader.cpp
    adios1/ADIOS1Writer.cpp
  )
  target_link_libraries(adios2 adios::adios)
endif()

if(ADIOS_USE_DataMan)
  find_pacakge(DataMan REQUIRED)
  target_sources(adios2 PRIVATE
    dataman/DataManReader.cpp
    dataman/DataManWriter.cpp
  )
  target_link_libraries(adios2 PRIVATE DataMan::DataMan)
endif()
