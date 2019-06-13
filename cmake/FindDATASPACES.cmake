#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindDATASPACES
# -----------
#
# Try to find the DataSpaces library
#
# This module defines the following variables:
#
#   DATASPACES_FOUND        - System has DataSpaces
#   DATASPACES_INCLUDE_DIRS - The DataSpaces include directory
#   DATASPACES_LIBRARIES    - Link these to use DataSpaces
#
# and the following imported targets:
#   DataSpaces::DataSpaces - The DataSpaces library target
#
# You can also set the following variable to help guide the search:
#   DATASPACES_ROOT - The install prefix for DataSpaces containing the
#              include and lib folders
#              Note: this can be set as a CMake variable or an
#                    environment variable.  If specified as a CMake
#                    variable, it will override any setting specified
#                    as an environment variable.

if(NOT DATASPACES_FOUND)
  if(NOT DATASPACES_ROOT)
    if(NOT ("$ENV{DATASPACES_ROOT}" STREQUAL ""))
      set(DATASPACES_ROOT "$ENV{DATASPACES_ROOT}")
    else()
      find_program(DSPACES_CONF dspaces_config)
      get_filename_component(DATASPACES_ROOT "${DSPACES_CONF}/../.." ABSOLUTE)
    endif()
  endif()
  if(DATASPACES_ROOT)
    find_program(DSPACES_CONF dspaces_config ${DATASPACES_ROOT}/bin)
    if(DSPACES_CONF)
	  execute_process(COMMAND ${DSPACES_CONF} -l
	    RESULT_VARIABLE RESULT_VAR
	    OUTPUT_VARIABLE DSPACES_CONFIG_STRING
	    ERROR_QUIET
	    OUTPUT_STRIP_TRAILING_WHITESPACE)
      string(REPLACE "-L"  ""   LINK_LIBS_ALL   ${DSPACES_CONFIG_STRING})
      string(REPLACE " -l"  ";"   LINK_LIBS   ${LINK_LIBS_ALL})
	  set(DATASPACES_LIBRARIES)
	  set(DATASPACES_LIBRARY_HINT)
	  foreach(LOOP_VAR ${LINK_LIBS})
	    STRING(FIND ${LOOP_VAR} "/" HINT_FLG)
		if(NOT("${HINT_FLG}" EQUAL "-1"))
		  string(REPLACE "-L" ";" INCLUDE_DIR ${LOOP_VAR})
		  list(APPEND DATASPACES_LIBRARY_HINT ${LOOP_VAR})
		else()
		  unset(LOCAL_LIBRARY CACHE)
		  STRING(FIND ${LOOP_VAR} "stdc++" CPP_FLG)
		  if("${CPP_FLG}" EQUAL "-1")
		    find_library(LOCAL_LIBRARY NAMES "${LOOP_VAR}" HINTS ${DATASPACES_LIBRARY_HINT})
			list(APPEND DATASPACES_LIBRARIES ${LOCAL_LIBRARY})
		  endif()
		endif()
      endforeach()
	  execute_process(COMMAND ${DSPACES_CONF} -v
        RESULT_VARIABLE RESULT_VAR
        OUTPUT_VARIABLE DATASPACES_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)	
	endif ()
	
	 set(DATASPACES_INCLUDE_OPTS HINTS ${DATASPACES_ROOT}/include)

  endif()

  find_path(DATASPACES_INCLUDE_DIR dataspaces.h ${DATASPACES_INCLUDE_OPTS})
  find_library(DSPACES_LIBRARY dspaces HINTS ${DATASPACES_LIBRARY_HINT}) 

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(DATASPACES
    FOUND_VAR DATASPACES_FOUND
    VERSION_VAR DATASPACES_VERSION
    REQUIRED_VARS DATASPACES_VERSION DATASPACES_INCLUDE_DIR 
      DATASPACES_LIBRARIES DSPACES_LIBRARY
  )
  if(DATASPACES_FOUND)
    if(DATASPACES_FOUND AND NOT TARGET DataSpaces::DataSpaces)
      add_library(DataSpaces::DataSpaces UNKNOWN IMPORTED)
      set_target_properties(DataSpaces::DataSpaces PROPERTIES
       	IMPORTED_LOCATION             "${DSPACES_LIBRARY}"
        INTERFACE_LINK_LIBRARIES      "${DATASPACES_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${DATASPACES_INCLUDE_DIR}"
      )
    endif()
  endif()
endif()
