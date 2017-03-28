add_library(adios2
  ADIOS.cpp
  #ADIOS_C.cpp
)
target_include_directories(adios2 PUBLIC ${ADIOS_SOURCE_DIR}/include)

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)
  target_include_directories(adios2 PUBLIC ${MPI_C_INCLUDE_PATH})
  target_link_libraries(adios2 PUBLIC ${MPI_C_LIBRARIES})
else()
  target_sources(adios2 mpidummy.cpp)
  target_compile_definitions(adios2 PUBLIC ADIOS_NOMPI)
endif()

add_subdirectory(capsule)
add_subdirectory(core)
add_subdirectory(engine)
add_subdirectory(format)
add_subdirectory(functions)
add_subdirectory(transport)
add_subdirectory(transform)
