add_subdirectory(KWSys)
add_subdirectory(NLohmannJson)

if(BUILD_TESTING)
  add_subdirectory(GTest)
endif()

if(ADIOS_USE_Python)
  add_subdirectory(pybind11)
endif()
