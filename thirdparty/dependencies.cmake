include(CMakeDependentOption)

add_subdirectory(thirdparty/KWSys)
add_subdirectory(thirdparty/pugixml)

if(BUILD_TESTING)
  add_subdirectory(thirdparty/GTest)
endif()

if(ADIOS2_HAVE_Python)
  add_subdirectory(thirdparty/pybind11)
endif()

option(ADIOS2_USE_SYSTEM_JSON
  "Use an externally supplied nlohmann-json library" OFF)

cmake_dependent_option(ADIOS2_USE_SYSTEM_EVPath
  "Use an externally supplied EVPath library" OFF
  "ADIOS2_HAVE_SST" OFF
)
cmake_dependent_option(ADIOS2_USE_SYSTEM_ATL
  "Use an externally supplied ATL library" OFF
  "NOT ADIOS2_USE_SYSTEM_EVPath" OFF
)
cmake_dependent_option(ADIOS2_USE_SYSTEM_FFS
  "Use an externally supplied FFS library" OFF
  "NOT ADIOS2_USE_SYSTEM_EVPath" OFF
)
cmake_dependent_option(ADIOS2_USE_SYSTEM_DILL
  "Use an externally supplied DILL library" OFF
  "NOT ADIOS2_USE_SYSTEM_FFS" OFF
)
cmake_dependent_option(ADIOS2_USE_SYSTEM_ENET
  "Use an externally supplied ENET library" OFF
  "NOT ADIOS2_USE_SYSTEM_EVPath" OFF
)

if(ADIOS2_USE_SYSTEM_JSON)
  find_package(nlohmann_json 3.1.2 REQUIRED)
else()
  add_subdirectory(thirdparty/NLohmannJson)
endif()

if(ADIOS2_HAVE_SST)
  if(NOT ADIOS2_USE_SYSTEM_EVPath)
    if(NOT ADIOS2_USE_SYSTEM_ATL)
      add_subdirectory(thirdparty/atl)
    endif()
    find_package(atl REQUIRED)

    if(NOT ADIOS2_USE_SYSTEM_FFS)
      if(NOT ADIOS2_USE_SYSTEM_DILL)
        add_subdirectory(thirdparty/dill)
      endif()
      find_package(dill REQUIRED)

      add_subdirectory(thirdparty/ffs)
    endif()
    find_package(ffs REQUIRED)

    if(NOT ADIOS2_USE_SYSTEM_ENET)
      add_subdirectory(thirdparty/enet)
    endif()
    find_package(enet REQUIRED)

    add_subdirectory(thirdparty/EVPath)
  endif()
  find_package(EVPath REQUIRED)
endif()
