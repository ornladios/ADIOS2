set(KWSYS_NAMESPACE adios2sys)
set(KWSYS_USE_DynamicLoader ON)
set(KWSYS_BUILD_SHARED OFF)
if(NOT ADIOS2_BUILD_SHARED_LIBS)
  set(KWSYS_INSTALL_EXPORT_NAME adios2)
  set(KWSYS_INSTALL_LIB_DIR ${CMAKE_INSTALL_LIBDIR})
endif()

add_subdirectory(adios2sys)

# Add the include usage requirements for KWSys
target_include_directories(adios2sys
  INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/adios2sys
)
