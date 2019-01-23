#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
  
add_library(adios2
  core/Attribute.cpp 
  core/AttributeBase.cpp
  core/ADIOS.cpp
  core/Engine.cpp
  core/IO.cpp core/IO.tcc
  core/Operator.cpp
  core/Stream.cpp core/Stream.tcc
  core/Variable.cpp 
  core/VariableBase.cpp
  core/VariableCompound.cpp core/VariableCompound.tcc

#operator callback
  operator/callback/Signature1.cpp
  operator/callback/Signature2.cpp

#helper
  helper/adiosDynamicBinder.h  helper/adiosDynamicBinder.cpp
  helper/adiosMath.cpp
  helper/adiosMemory.cpp helper/adiosMemory.tcc
  helper/adiosMPIFunctions.cpp
  helper/adiosString.cpp
  helper/adiosSystem.cpp
  helper/adiosType.cpp
  helper/adiosXML.cpp

#engine derived classes  
  engine/bp3/BP3Reader.cpp engine/bp3/BP3Reader.tcc
  engine/bp3/BP3Writer.cpp engine/bp3/BP3Writer.tcc

  engine/skeleton/SkeletonReader.cpp engine/skeleton/SkeletonReader.tcc
  engine/skeleton/SkeletonWriter.cpp engine/skeleton/SkeletonWriter.tcc

#toolkit
  toolkit/format/BufferSTL.cpp
  
  toolkit/format/bp3/BP3Base.cpp toolkit/format/bp3/BP3Base.tcc
  toolkit/format/bp3/BP3Serializer.cpp toolkit/format/bp3/BP3Serializer.tcc
  toolkit/format/bp3/BP3Deserializer.cpp toolkit/format/bp3/BP3Deserializer.tcc
  toolkit/format/bp3/operation/BP3Operation.cpp
  toolkit/format/bp3/operation/BP3Zfp.cpp
  toolkit/format/bp3/operation/BP3Zfp.tcc
  toolkit/format/bp3/operation/BP3SZ.cpp
  toolkit/format/bp3/operation/BP3SZ.tcc
  toolkit/format/bp3/operation/BP3MGARD.cpp
  toolkit/format/bp3/operation/BP3MGARD.tcc

  toolkit/profiling/iochrono/Timer.cpp

  toolkit/transport/Transport.cpp
  toolkit/transport/file/FileStdio.cpp
  toolkit/transport/file/FileFStream.cpp
  
  toolkit/transportman/TransportMan.cpp
  
  toolkit/aggregator/mpi/MPIAggregator.cpp
  toolkit/aggregator/mpi/MPIChain.cpp
  
)
target_include_directories(adios2
  PUBLIC
    $<BUILD_INTERFACE:${ADIOS2_SOURCE_DIR}/source>
    $<BUILD_INTERFACE:${ADIOS2_BINARY_DIR}/source>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(adios2 PRIVATE adios2sys_interface pugixml) 
target_link_libraries(adios2 PUBLIC ${CMAKE_THREAD_LIBS_INIT})
target_compile_features(adios2 PUBLIC ${ADIOS2_CXX11_FEATURES})

if(UNIX)
  target_sources(adios2 PRIVATE toolkit/transport/file/FilePOSIX.cpp)
endif()

if(ADIOS2_HAVE_SysVShMem)
  target_sources(adios2 PRIVATE toolkit/transport/shm/ShmSystemV.cpp)
endif()

if(ADIOS2_HAVE_ZeroMQ)
    target_sources(adios2 PRIVATE
        toolkit/transport/wan/WANZmqPubSub.cpp
        toolkit/transport/wan/WANZmqPubSub.h
        toolkit/transport/wan/WANZmqP2P.cpp
        toolkit/transport/wan/WANZmqP2P.h
        )
    target_link_libraries(adios2 PRIVATE ZeroMQ::ZMQ)
endif()

if(ADIOS2_HAVE_DataMan)
  target_sources(adios2 PRIVATE
      toolkit/format/dataman/DataManDeserializer.cpp
      toolkit/format/dataman/DataManDeserializer.tcc
      toolkit/format/dataman/DataManDeserializer.h
      toolkit/format/dataman/DataManSerializer.cpp
      toolkit/format/dataman/DataManSerializer.tcc
      toolkit/format/dataman/DataManSerializer.h
      toolkit/transportman/dataman/DataMan.cpp
      toolkit/transportman/dataman/DataMan.h
      engine/dataman/DataManCommon.cpp
      engine/dataman/DataManCommon.h
      engine/dataman/DataManReader.cpp
      engine/dataman/DataManReader.h
      engine/dataman/DataManReader.tcc
      engine/dataman/DataManWriter.cpp
      engine/dataman/DataManWriter.h
      engine/dataman/DataManWriter.tcc
  )
  target_link_libraries(adios2 PRIVATE nlohmann_json)
endif()

if(ADIOS2_HAVE_SST)
  add_subdirectory(toolkit/sst)
  target_sources(adios2 PRIVATE
    engine/sst/SstReader.cpp
    engine/sst/SstWriter.cpp
    engine/sst/SstParamParser.cpp
  )
  target_link_libraries(adios2 PRIVATE sst)
endif()


if(ADIOS2_HAVE_BZip2)
  target_sources(adios2 PRIVATE operator/compress/CompressBZip2.cpp)
  target_link_libraries(adios2 PRIVATE BZip2::BZip2)
endif()

if(ADIOS2_HAVE_ZFP)
  target_sources(adios2 PRIVATE operator/compress/CompressZfp.cpp)
  target_link_libraries(adios2 PRIVATE zfp::zfp)
endif()

if(ADIOS2_HAVE_SZ)
  target_sources(adios2 PRIVATE operator/compress/CompressSZ.cpp)
  target_link_libraries(adios2 PRIVATE SZ::SZ)
endif()

if(ADIOS2_HAVE_MGARD)
  target_sources(adios2 PRIVATE operator/compress/CompressMGARD.cpp)
  target_link_libraries(adios2 PRIVATE MGARD::MGARD)
endif()

if(ADIOS2_HAVE_MPI)
  target_sources(adios2 PRIVATE
    engine/insitumpi/InSituMPIWriter.cpp engine/insitumpi/InSituMPIWriter.tcc
    engine/insitumpi/InSituMPIReader.cpp engine/insitumpi/InSituMPIReader.tcc
    engine/insitumpi/InSituMPIFunctions.cpp engine/insitumpi/InSituMPISchedules.cpp
  )
  target_link_libraries(adios2 PUBLIC MPI::MPI_C)
else()
  target_sources(adios2 PRIVATE helper/mpidummy.cpp)
endif()

if(ADIOS2_HAVE_HDF5)
  if(HDF5_C_INCLUDE_DIRS)
    target_include_directories(adios2 PRIVATE ${HDF5_C_INCLUDE_DIRS})
  else()
    target_include_directories(adios2 PRIVATE ${HDF5_INCLUDE_DIRS})
  endif()

  if(HDF5_VERSION VERSION_LESS 1.11)
    target_sources(adios2 PRIVATE
      engine/hdf5/HDF5ReaderP.cpp
      engine/hdf5/HDF5WriterP.cpp
      toolkit/interop/hdf5/HDF5Common.cpp toolkit/interop/hdf5/HDF5Common.tcc
    )  
  else()
    target_sources(adios2 PRIVATE
      engine/hdf5/HDF5ReaderP.cpp
      engine/hdf5/HDF5WriterP.cpp
      engine/mixer/HDFMixer.cpp	
      engine/mixer/HDFMixer.tcc
      engine/mixer/HDFMixerWriter.cpp
      toolkit/interop/hdf5/HDF5Common.cpp toolkit/interop/hdf5/HDF5Common.tcc
    )
  endif()

  target_link_libraries(adios2 PRIVATE ${HDF5_C_LIBRARIES})
endif()

# Set library version information
set_target_properties(adios2 PROPERTIES
  VERSION ${ADIOS2_VERSION}
  SOVERSION ${ADIOS2_VERSION_MAJOR}
)

install(FILES ADIOSMacros.h ADIOSTypes.h ADIOSMPICommOnly.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2
)

# Library installation
install(TARGETS adios2 EXPORT adios2Exports
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
