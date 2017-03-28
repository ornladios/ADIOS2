target_sources(adios2 PRIVATE
  transport/file/FStream.cpp
  transport/file/FileDescriptor.cpp
  transport/file/FilePointer.cpp
)

if(ADIOS_USE_DataMan)
  find_pacakge(DataMan REQUIRED)
  target_sources(adios2 PRIVATE
    wan/MdtmMan.cpp
  )
  target_link_libraries(adios2 PRIVATE DataMan::DataMan)
endif()
