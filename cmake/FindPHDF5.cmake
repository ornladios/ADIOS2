#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindPHDF5
# ---------
#


if(NOT PHDF5_FOUND)
  if((NOT HDF5_DIR) AND (NOT (ENV{HDF5_DIR} STREQUAL "")))
    set(HDF5_DIR "$ENV{HDF5_DIR}")
  endif()

  # Search for the core libraries
  if(HDF5_DIR)
    # If a root directory is specified, then don't look anywhere else 
    find_path(PHDF5_INCLUDE_DIR hdf5.h
      HINTS ${HDF5_DIR}/include
      NO_DEFAULT_PATHS
    )
    set(_PHDF5_LIBRARY_HINT HINTS ${HDF5_DIR}/lib NO_DEFAULT_PATHS)
  else()
    # Otherwise use the include dir as a basis to search for the lib
    find_path(PHDF5_INCLUDE_DIR hdf5.h)
    if(PHDF5_INCLUDE_DIR)
      get_filename_component(_PHDF5_PREFIX "${PHDF5_INCLUDE_DIR}" PATH)
      set(_PHDF5_LIBRARY_HINT HINTS ${_PHDF5_PREFIX}/lib)
      unset(_PHDF5_PREFIX)
    endif()
  endif()
  find_library(PHDF5_LIBRARY hdf5 ${_PHDF5_LIBRARY_HINT})
  unset(_PHDF5_LIBRARY_HINT)


  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PHDF5  DEFAULT_MSG  PHDF5_LIBRARY PHDF5_INCLUDE_DIR)

  find_package_handle_standard_args(PHDF5
    FOUND_VAR PHDF5_FOUND
    REQUIRED_VARS
      PHDF5_INCLUDE_DIR
      PHDF5_LIBRARY
  )

  MESSAGE(STATUS "PHDF5_FOUND: " ${PHDF5_FOUND} "HDF5_DIR: " ${HDF5_DIR} "TARGET=" ${TARGET} )
  if(PHDF5_FOUND)
    set(PHDF5_INCLUDE_DIRS ${PHDF5_INCLUDE_DIR})
    set(PHDF5_LIBRARIES ${PHDF5_LIBRARY})
    if(PHDF5_FOUND AND NOT TARGET PHDF5::PHDF5)
      add_library(PHDF5::PHDF5 UNKNOWN IMPORTED)
      set_target_properties(PHDF5::PHDF5 PROPERTIES
        IMPORTED_LOCATION "${PHDF5_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PHDF5_INCLUDE_DIR}"
      )
    endif()
  endif()
endif()
