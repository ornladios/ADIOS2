#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_library(adios2_cxx
  adios2/cxx/ADIOS.cpp
  adios2/cxx/ADIOSView.h
  adios2/cxx/Attribute.cpp
  adios2/cxx/Engine.cpp
  adios2/cxx/Engine.tcc
  adios2/cxx/IO.cpp
  adios2/cxx/IO.tcc
  adios2/cxx/Operator.cpp
  adios2/cxx/Query.cpp
  adios2/cxx/Types.cpp
  adios2/cxx/Types.tcc
  adios2/cxx/Variable.cpp
  adios2/cxx/VariableNT.cpp
  adios2/cxx/fstream/ADIOS2fstream.cpp
  adios2/cxx/Group.cpp
  adios2/cxx/Group.tcc
)

set_property(TARGET adios2_cxx PROPERTY EXPORT_NAME cxx)
set_property(TARGET adios2_cxx PROPERTY OUTPUT_NAME adios2${ADIOS2_LIBRARY_SUFFIX}_cxx)
target_link_libraries(adios2_cxx PRIVATE adios2_core)
target_compile_features(adios2_cxx INTERFACE ${ADIOS2_CXX11_FEATURES})

target_include_directories(adios2_cxx
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  INTERFACE
    $<BUILD_INTERFACE:$<TARGET_PROPERTY:adios2_core,INTERFACE_INCLUDE_DIRECTORIES>>
)

# Generate deprecated cxx11 wrapper headers at configure time
set(CXX11_WRAPPER_HEADERS
  ADIOS.h
  ADIOS.inl
  ADIOSView.h
  Attribute.h
  Engine.h
  Group.h
  IO.h
  KokkosView.h
  Operator.h
  Query.h
  Types.h
  Variable.h
  VariableNT.h
)

if(ADIOS2_HAVE_Derived_Variable)
  list(APPEND CXX11_WRAPPER_HEADERS VariableDerived.h)
endif()

foreach(header ${CXX11_WRAPPER_HEADERS})
  set(INCLUDE_PATH "../cxx/${header}")
  string(TOUPPER "${header}" HEADER_NAME)
  string(REPLACE "." "_" HEADER_NAME "${HEADER_NAME}")
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cxx11_wrapper.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/adios2/cxx11/${header}
    @ONLY
  )
endforeach()

# Generate fstream wrapper header
set(INCLUDE_PATH "../../cxx/fstream/ADIOS2fstream.h")
set(HEADER_NAME "ADIOS2FSTREAM_H")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cxx11_wrapper.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/adios2/cxx11/fstream/ADIOS2fstream.h
  @ONLY
)
unset(INCLUDE_PATH)
unset(HEADER_NAME)

add_library(adios2::cxx ALIAS adios2_cxx)

if (ADIOS2_HAVE_Derived_Variable)
    target_sources(adios2_cxx PRIVATE
      adios2/cxx/VariableDerived.cpp
  )
endif()

if(ADIOS2_HAVE_MPI)
  add_library(adios2_cxx_mpi
    adios2/cxx/ADIOSMPI.cpp
    adios2/cxx/IOMPI.cpp
    adios2/cxx/fstream/ADIOS2fstreamMPI.cpp
  )
  set_property(TARGET adios2_cxx_mpi PROPERTY EXPORT_NAME cxx_mpi)
  set_property(TARGET adios2_cxx_mpi PROPERTY OUTPUT_NAME adios2${ADIOS2_LIBRARY_SUFFIX}_cxx_mpi)
  target_link_libraries(adios2_cxx_mpi PUBLIC adios2_cxx PRIVATE adios2_core_mpi PUBLIC MPI::MPI_CXX)
  set(maybe_adios2_cxx_mpi adios2_cxx_mpi)
  target_compile_definitions(adios2_cxx_mpi INTERFACE ADIOS2_USE_MPI)
  add_library(adios2::cxx_mpi ALIAS adios2_cxx_mpi)
else()
  set(maybe_adios2_cxx_mpi)
endif()

# Set library version information
set_target_properties(
  adios2_cxx ${maybe_adios2_cxx_mpi}
  PROPERTIES
  VERSION ${ADIOS2_LIBRARY_VERSION}
  SOVERSION ${ADIOS2_LIBRARY_SOVERSION}
)

install(TARGETS adios2_cxx ${maybe_adios2_cxx_mpi} EXPORT adios2CXXExports
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT adios2_cxx-runtime
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT adios2_cxx-libraries NAMELINK_COMPONENT adios2_cxx-development
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT adios2_cxx-development
)

install(
  FILES adios2.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  COMPONENT adios2_cxx-development
)

install(
  FILES adios2/cxx/fstream/ADIOS2fstream.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2/cxx/fstream
  COMPONENT adios2_cxx-development
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/adios2/cxx11/fstream/ADIOS2fstream.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2/cxx11/fstream
  COMPONENT adios2_cxx11-development
)

if (ADIOS2_HAVE_Derived_Variable)
    install(
        FILES adios2/cxx/VariableDerived.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2/cxx
        COMPONENT adios2_cxx-development
    )
endif()

install(
  FILES adios2/cxx/ADIOS.h
        adios2/cxx/ADIOS.inl
        adios2/cxx/ADIOSView.h
        adios2/cxx/IO.h
        adios2/cxx/Group.h
        adios2/cxx/Variable.h
        adios2/cxx/VariableNT.h
        adios2/cxx/Attribute.h
        adios2/cxx/Engine.h
        adios2/cxx/KokkosView.h
        adios2/cxx/Operator.h
        adios2/cxx/Query.h
        adios2/cxx/Types.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2/cxx
  COMPONENT adios2_cxx-development
)

list(TRANSFORM CXX11_WRAPPER_HEADERS
     PREPEND "${CMAKE_CURRENT_BINARY_DIR}/adios2/cxx11/"
     OUTPUT_VARIABLE GENERATED_CXX11_HEADERS
)

install(
  FILES ${GENERATED_CXX11_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2/cxx11
  COMPONENT adios2_cxx-development
)

export(EXPORT adios2CXXExports
  FILE ${ADIOS2_BINARY_DIR}/adios2-cxx-targets.cmake
  NAMESPACE adios2::
)
install(EXPORT adios2CXXExports
  FILE adios2-cxx-targets.cmake
  NAMESPACE adios2::
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
  COMPONENT adios2_cxx-development
)
