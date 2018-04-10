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
