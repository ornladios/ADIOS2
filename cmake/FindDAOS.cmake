# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0


if((NOT DAOS_ROOT) AND (NOT (ENV{DAOS_ROOT} STREQUAL "")))
  set(DAOS_ROOT "$ENV{DAOS_ROOT}")
endif()
if(DAOS_ROOT)
  set(DAOS_INCLUDE_OPTS HINTS ${DAOS_ROOT}/include NO_DEFAULT_PATHS)
  set(DAOS_LIBRARY_OPTS
    HINTS ${DAOS_ROOT}/lib ${DAOS_ROOT}/lib64
    NO_DEFAULT_PATHS
    )
endif()

find_path(DAOS_INCLUDE_DIR daos_api.h ${DAOS_INCLUDE_OPTS})
find_library(DAOS_LIBRARY libdaos.so ${DAOS_LIBRARY_OPTS})
find_library(DFS_LIBRARY libdfs.so ${DAOS_LIBRARY_OPTS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DAOS
  FOUND_VAR DAOS_FOUND
  REQUIRED_VARS DAOS_LIBRARY DFS_LIBRARY DAOS_INCLUDE_DIR
)

if(DAOS_FOUND)
  set(DAOS_INCLUDE_DIRS ${DAOS_INCLUDE_DIR})
  set(DAOS_LIBRARIES ${DAOS_LIBRARY} ${DFS_LIBRARY})
  message(STATUS "DAOS Libraries \"${DAOS_LIBRARIES}\"")
  if(NOT TARGET DAOS::DAOS)
    add_library(DAOS::DAOS UNKNOWN IMPORTED)
    set_target_properties(DAOS::DAOS PROPERTIES
      IMPORTED_LOCATION             "${DAOS_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${DAOS_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES      "${DAOS_LIBRARIES}"
    )
  endif()
endif()
