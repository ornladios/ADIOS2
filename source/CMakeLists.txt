#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
  
add_library(adios2
  ADIOS.cpp ADIOS.tcc
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

  utilities/format/bp1/BP1Base.cpp
  utilities/format/bp1/BP1Aggregator.cpp
  utilities/format/bp1/BP1Writer.cpp
    
  utilities/profiling/iochrono/Timer.cpp

  functions/adiosFunctions.cpp
  
  transport/file/FStream.cpp
  transport/file/FileDescriptor.cpp
  transport/file/FilePointer.cpp
    
  utilities/format/bp1/BP1Base.cpp
  utilities/format/bp1/BP1Aggregator.cpp
  utilities/format/bp1/BP1Writer.cpp
  utilities/format/bp1/BP1Writer.tcc
    
  utilities/profiling/iochrono/Timer.cpp
)
target_include_directories(adios2
  PUBLIC ${ADIOS_SOURCE_DIR}/include
)
  
if(ADIOS_USE_DataMan)
  target_sources(adios2 PRIVATE
    engine/dataman/DataManReader.cpp
    engine/dataman/DataManWriter.cpp
    transport/wan/MdtmMan.cpp
    utilities/realtime/dataman/DataManBase.cpp
    utilities/realtime/dataman/DataMan.cpp
  )
  target_compile_definitions(adios2 PRIVATE ADIOS_HAVE_DATAMAN)
  target_link_libraries(adios2 PRIVATE ${CMAKE_DL_LIBS})
endif()

  
if(ADIOS_USE_BZip2)
  find_package(BZip2 REQUIRED)
  target_sources(adios2 PRIVATE transform/BZip2.cpp)
  target_compile_definitions(adios2 PRIVATE ADIOS_HAVE_BZIP2)
  target_link_libraries(adios2 PRIVATE BZip2::BZip2)
endif()


if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)
  target_include_directories(adios2 PUBLIC ${MPI_C_INCLUDE_PATH})
  target_compile_definitions(adios2 PUBLIC ADIOS_HAVE_MPI)
  target_link_libraries(adios2 PUBLIC ${MPI_C_LIBRARIES})
else()
  target_sources(adios2 PRIVATE mpidummy.cpp)
endif()

if(ADIOS_USE_ADIOS1)
  if(ADIOS_HAVE_MPI)
    find_package(ADIOS1 REQUIRED)
  else()
    find_package(ADIOS1 COMPONENTS sequential REQUIRED)
  endif()

  target_sources(adios2 PRIVATE
    engine/adios1/ADIOS1Reader.cpp
    engine/adios1/ADIOS1Writer.cpp
  )
  target_compile_definitions(adios2 PRIVATE ADIOS_HAVE_ADIOS1)
  target_link_libraries(adios2 PRIVATE adios1::adios)
endif()

if(ADIOS_USE_HDF5)
  find_package(HDF5 REQUIRED)
  if(ADIOS_USE_MPI AND NOT HDF5_IS_PARALLEL)
    message(FATAL_ERROR "A sequential version of HDF5 was detected but the parallel version of ADIOS requires a parallel HDF5.")
  endif()

  target_include_directories(adios2 PRIVATE ${HDF5_INCLUDE_DIRS})
  target_sources(adios2 PRIVATE
    engine/hdf5/HDF5ReaderP.cpp
    engine/hdf5/HDF5WriterP.cpp
  )
  target_compile_definitions(adios2 PRIVATE ADIOS_HAVE_HDF5)
  target_link_libraries(adios2 PRIVATE ${HDF5_C_LIBRARIES})
endif()
