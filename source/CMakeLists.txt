#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_library(adios2
  ADIOS.cpp
  #ADIOS_C.cpp

  capsule/heap/STLVector.cpp
  capsule/shmem/ShmSystemV.cpp

  core/Capsule.cpp
  core/Engine.cpp
  core/Method.cpp
  core/Support.cpp
  core/Transform.cpp
  core/Transport.cpp

  engine/bp/BPFileReader.cpp
  engine/bp/BPFileWriter.cpp

  format/BP1.cpp
  format/BP1Aggregator.cpp
  format/BP1Writer.cpp

  functions/adiosFunctions.cpp

  transport/file/FStream.cpp
  transport/file/FileDescriptor.cpp
  transport/file/FilePointer.cpp
)
target_include_directories(adios2 PUBLIC ${ADIOS_SOURCE_DIR}/include)

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)
  target_include_directories(adios2 PUBLIC ${MPI_C_INCLUDE_PATH})
  target_link_libraries(adios2 PUBLIC ${MPI_C_LIBRARIES})
else()
  target_sources(adios2 PRIVATE mpidummy.cpp)
  target_compile_definitions(adios2 PUBLIC ADIOS_NOMPI)
endif()

if(ADIOS_USE_ADIOS1)
  find_package(ADIOS1 REQUIRED)
  target_sources(adios2 PRIVATE
    engine/adios1/ADIOS1Reader.cpp
    engine/adios1/ADIOS1Writer.cpp
  )
  target_link_libraries(adios2 PRIVATE adios1::adios)
endif()

if(ADIOS_USE_DataMan)
  find_package(DataMan REQUIRED)
  target_sources(adios2 PRIVATE
    engine/dataman/DataManReader.cpp
    engine/dataman/DataManWriter.cpp
    transport/wan/MdtmMan.cpp
  )
  target_link_libraries(adios2 PRIVATE DataMan::DataMan)
endif()

if(ADIOS_USE_BZip2)
  find_package(BZip2 REQUIRED)
  target_sources(adios2 PRIVATE transform/BZip2.cpp)
  target_link_libraries(adios2 PRIVATE BZip2::BZip2)
endif()
