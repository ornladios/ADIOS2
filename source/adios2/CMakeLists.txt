#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
  
add_library(adios2
  ADIOS.cpp ADIOS.tcc
  
  capsule/heap/STLVector.cpp
  capsule/shmem/ShmSystemV.cpp
  
  core/Capsule.cpp
  core/Engine.cpp
  core/Method.cpp
  core/Support.cpp
  core/Timer.cpp
  core/Transform.cpp
  core/Transport.cpp
  core/adiosFunctions.cpp
  
  engine/bp/BPFileReader.cpp
  engine/bp/BPFileWriter.cpp

  utilities/format/bp1/BP1Base.cpp
  utilities/format/bp1/BP1Aggregator.cpp
  utilities/format/bp1/BP1Writer.cpp
    
  transport/file/FStream.cpp
  transport/file/FileDescriptor.cpp
  transport/file/FilePointer.cpp
    
  utilities/format/bp1/BP1Base.cpp
  utilities/format/bp1/BP1Aggregator.cpp
  utilities/format/bp1/BP1Writer.cpp
  utilities/format/bp1/BP1Writer.tcc
)
target_include_directories(adios2
  PUBLIC
    $<BUILD_INTERFACE:${ADIOS_SOURCE_DIR}/source>
    $<BUILD_INTERFACE:${ADIOS_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>
  PRIVATE ${ADIOS_SOURCE_DIR}/source
)

find_package(Threads REQUIRED)
target_link_libraries(adios2 PUBLIC ${CMAKE_THREAD_LIBS_INIT})

if(ADIOS_USE_DataMan)
  target_sources(adios2 PRIVATE
    engine/dataman/DataManReader.cpp
    engine/dataman/DataManWriter.cpp
    transport/wan/MdtmMan.cpp
  )
  target_link_libraries(adios2 PRIVATE dataman)
endif()
  
if(ADIOS_USE_BZip2)
  find_package(BZip2 REQUIRED)
  target_sources(adios2 PRIVATE transform/BZip2.cpp)
  target_link_libraries(adios2 PRIVATE BZip2::BZip2)
endif()

if(ADIOS_USE_ZFP)
  find_package(ZFP REQUIRED)
  message("ADIOS ZFP support not yet implemented")
#  target_sources(adios2 PRIVATE transform/ZFP.cpp)
#  target_link_libraries(adios2 PRIVATE zfp::zfp)
endif()

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)
  target_include_directories(adios2 PUBLIC ${MPI_C_INCLUDE_PATH})
  target_link_libraries(adios2 PUBLIC ${MPI_C_LIBRARIES})
else()
  target_sources(adios2 PRIVATE mpidummy.cpp)
endif()

if(ADIOS_USE_ADIOS1)
  if(ADIOS_USE_MPI)
    find_package(ADIOS1 REQUIRED)
  else()
    find_package(ADIOS1 COMPONENTS sequential REQUIRED)
  endif()

  target_sources(adios2 PRIVATE
    engine/adios1/ADIOS1Reader.cpp
    engine/adios1/ADIOS1Writer.cpp
  )
  target_link_libraries(adios2 PRIVATE adios1::adios)
endif()

if(ADIOS_USE_HDF5)
  find_package(HDF5 REQUIRED)
  if(ADIOS_USE_MPI AND (NOT HDF5_IS_PARALLEL))
    message(FATAL_ERROR
      "A sequential version of HDF5 was detected but the parallel version "
      "of ADIOS is being built, which requires a parallel HDF5."
    )
  elseif((NOT ADIOS_USE_MPI) AND HDF5_IS_PARALLEL)
    message(FATAL_ERROR
      "A parallel version of HDF5 was detected but the sequential version "
      "of ADIOS is being built, which requires a sequential HDF5."
    )
  endif()
  target_include_directories(adios2 PRIVATE ${HDF5_C_INCLUDE_DIRS})
  target_sources(adios2 PRIVATE
    engine/hdf5/HDF5ReaderP.cpp
    engine/hdf5/HDF5WriterP.cpp
    engine/hdf5/HDF5Common.cpp
  )
  target_link_libraries(adios2 PRIVATE ${HDF5_C_LIBRARIES})
endif()

install(
  FILES ADIOS.h ADIOS.inl ADIOSMacros.h ADIOSTypes.h ADIOSMPICommOnly.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2
)
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/core
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2
  FILES_MATCHING REGEX "[^/]*\.(h|inl)$"
)

# Library installation
install(
  TARGETS adios2 EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
