add_subdirectory(KWSys)
add_subdirectory(NLohmannJson)
add_subdirectory(pugixml)

if(BUILD_TESTING)
  add_subdirectory(GTest)
endif()

if(ADIOS2_HAVE_Python)
  add_subdirectory(pybind11)
endif()
