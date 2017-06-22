set(KWSYS_NAMESPACE adios2sys)
set(KWSYS_USE_DynamicLoader ON)
set(KWSYS_USE_RegularExpression ON)
set(KWSYS_USE_SystemTools ON)
set(KWSYS_BUILD_SHARED ${ADIOS2_BUILD_SHARED_LIBS})
set(KWSYS_SPLIT_OBJECTS_FROM_INTERFACE ON)

add_subdirectory(adios2sys EXCLUDE_FROM_ALL)
