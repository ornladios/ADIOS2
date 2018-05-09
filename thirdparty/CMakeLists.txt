include(CMakeDependentOption)

add_subdirectory(KWSys)

if(BUILD_TESTING)
  add_subdirectory(GTest)
endif()

if(ADIOS2_HAVE_Python)
  add_subdirectory(pybind11)
endif()

add_subdirectory(pugixml)

if(WIN32)
  add_subdirectory(mingw-w64)
endif()

option(ADIOS2_USE_EXTERNAL_JSON
  "Use an externally supplied nlohmann_json library" OFF
)
if(NOT ADIOS2_USE_EXTERNAL_JSON)
  add_subdirectory(nlohmann_json)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
       CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9")
  message(STATUS "WARNING - The upstream nlohmann_json library is known to")
  message(STATUS "WARNING - have problems with GCC < 4.9 so the version")
  message(STATUS "WARNING - included in ADIOS2 is patched to allow it's use.")
  message(STATUS "WARNING - if you encounter json related build failures")
  message(STATUS "WARNING - disable the use of an externally provided JSON")
  message(STATUS "WARNING - by setting ADIOS2_USE_EXTERNAL_JSON=OFF.")
endif()
find_package(nlohmann_json REQUIRED)
adios2_add_thirdparty_target(nlohmann_json nlohmann_json::nlohmann_json)

#------------------------------------------------------------------------------#
# SST / EVPath stuff
#------------------------------------------------------------------------------#
cmake_dependent_option(ADIOS2_USE_EXTERNAL_EVPATH
  "Use an externally supplied EVPath library" OFF
  "ADIOS2_HAVE_SST" OFF
)
cmake_dependent_option(ADIOS2_USE_EXTERNAL_ATL
  "Use an externally supplied ATL library" OFF
  "NOT ADIOS2_USE_EXTERNAL_EVPATH" OFF
)
cmake_dependent_option(ADIOS2_USE_EXTERNAL_FFS
  "Use an externally supplied FFS library" OFF
  "NOT ADIOS2_USE_EXTERNAL_EVPATH" OFF
)
cmake_dependent_option(ADIOS2_USE_EXTERNAL_DILL
  "Use an externally supplied DILL library" OFF
  "NOT ADIOS2_USE_EXTERNAL_FFS" OFF
)
cmake_dependent_option(ADIOS2_USE_EXTERNAL_ENET
  "Use an externally supplied ENET library" OFF
  "NOT ADIOS2_USE_EXTERNAL_EVPATH" OFF
)

if(ADIOS2_HAVE_SST)
  if(NOT ADIOS2_USE_EXTERNAL_EVPATH)
    if(NOT ADIOS2_USE_EXTERNAL_ATL)
      add_subdirectory(atl)
    endif()
    find_package(atl REQUIRED)

    if(NOT ADIOS2_USE_EXTERNAL_FFS)
      if(NOT ADIOS2_USE_EXTERNAL_DILL)
        add_subdirectory(dill)
      endif()
      find_package(dill REQUIRED)

      add_subdirectory(ffs)
    endif()
    find_package(ffs REQUIRED)

    if(NOT ADIOS2_USE_EXTERNAL_ENET)
      add_subdirectory(enet)
    endif()
    find_package(enet REQUIRED)

    add_subdirectory(EVPath)
  endif()
  adios2_add_thirdparty_target(EVPath EVPath::EVPath)
endif()
