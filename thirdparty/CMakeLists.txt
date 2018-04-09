include(CMakeDependentOption)

add_subdirectory(KWSys)
add_subdirectory(NLohmannJson)
add_subdirectory(pugixml)

if(BUILD_TESTING)
  add_subdirectory(GTest)
endif()

if(ADIOS2_HAVE_Python)
  add_subdirectory(pybind11)
endif()

cmake_dependent_option(ADIOS2_USE_SYSTEM_ZFP
  "Use an externally supplied ZFP library" OFF
  "ADIOS2_HAVE_ZFP" OFF
)
if(ADIOS2_HAVE_ZFP)
  if(NOT ADIOS2_USE_SYSTEM_ZFP)
    add_subdirectory(zfp)
  endif()
  find_package(zfp REQUIRED)
endif()

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

if(ADIOS2_HAVE_SST)
  if(NOT ADIOS2_USE_SYSTEM_EVPath)
    if(NOT ADIOS2_USE_SYSTEM_ATL)
      add_subdirectory(atl)
    endif()
    find_package(atl REQUIRED)

    if(NOT ADIOS2_USE_SYSTEM_FFS)
      if(NOT ADIOS2_USE_SYSTEM_DILL)
        add_subdirectory(dill)
      endif()
      find_package(dill REQUIRED)

      add_subdirectory(ffs)
    endif()
    find_package(ffs REQUIRED)

    if(NOT ADIOS2_USE_SYSTEM_ENET)
      add_subdirectory(enet)
    endif()
    find_package(enet REQUIRED)

    add_subdirectory(EVPath)
  endif()
  find_package(EVPath REQUIRED)
endif()
