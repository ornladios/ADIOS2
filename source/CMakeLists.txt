#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
  
set(adios2_targets adios2_nompi)
if(ADIOS_USE_MPI)
  list(APPEND adios2_targets adios2)
endif()
  
foreach(adios2_target IN LISTS adios2_targets)
  add_library(${adios2_target}
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
  target_include_directories(${adios2_target}
    PUBLIC ${ADIOS_SOURCE_DIR}/include
  )
  
  if(ADIOS_USE_DataMan)
    target_sources(${adios2_target} PRIVATE
      engine/dataman/DataManReader.cpp
      engine/dataman/DataManWriter.cpp
      transport/wan/MdtmMan.cpp
      utilities/realtime/dataman/DataManBase.cpp
      utilities/realtime/dataman/DataMan.cpp
    )
    target_compile_definitions(${adios2_target} PRIVATE ADIOS_HAVE_DATAMAN)
    target_link_libraries(${adios2_target} PRIVATE ${CMAKE_DL_LIBS})
  endif()
  
  if(ADIOS_USE_BZip2)
    find_package(BZip2 REQUIRED)
    target_sources(${adios2_target} PRIVATE transform/BZip2.cpp)
    target_compile_definitions(${adios2_target} PRIVATE ADIOS_HAVE_BZIP2)
    target_link_libraries(${adios2_target} PRIVATE BZip2::BZip2)
  endif()
endforeach()

target_sources(adios2_nompi PRIVATE mpidummy.cpp)
if(CMAKE_CXX_COMPILER_WRAPPER STREQUAL CrayPrgEnv)
  target_compile_options(adios2_nompi PRIVATE --cray-bypass-pkgconfig)
endif()

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)
  target_include_directories(adios2 PUBLIC ${MPI_C_INCLUDE_PATH})
  target_compile_definitions(adios2 PUBLIC ADIOS_HAVE_MPI)
  target_link_libraries(adios2 PUBLIC ${MPI_C_LIBRARIES})

  if(ADIOS_USE_ADIOS1)
    find_package(ADIOS1 REQUIRED)
    target_sources(adios2 PRIVATE
      engine/adios1/ADIOS1Reader.cpp
      engine/adios1/ADIOS1Writer.cpp
    )
    target_compile_definitions(adios2 PRIVATE ADIOS_HAVE_ADIOS1)
    target_link_libraries(adios2 PRIVATE adios1::adios)
  endif()
endif()
