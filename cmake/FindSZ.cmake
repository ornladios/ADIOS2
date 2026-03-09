# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0


if(NOT SZ_FOUND)
  if((NOT SZ_ROOT) AND (NOT (ENV{SZ_ROOT} STREQUAL "")))
    set(SZ_ROOT "$ENV{SZ_ROOT}")
  endif()
  if(SZ_ROOT)
    set(SZ_INCLUDE_OPTS HINTS ${SZ_ROOT}/include NO_DEFAULT_PATHS)
    set(SZ_LIBRARY_OPTS
      HINTS ${SZ_ROOT}/lib ${SZ_ROOT}/lib64
      NO_DEFAULT_PATHS
    )
  endif()

  find_path(SZ_INCLUDE_DIR sz.h PATH_SUFFIXES sz ${SZ_INCLUDE_OPTS})
  find_library(SZ_LIBRARY NAMES SZ ${SZ_LIBRARY_OPTS})
  find_library(ZLIB_LIBRARY NAMES z zlib ${SZ_LIBRARY_OPTS})
  find_library(ZSTD_LIBRARY NAMES zstd ${SZ_LIBRARY_OPTS})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(SZ
    FOUND_VAR SZ_FOUND
    REQUIRED_VARS SZ_LIBRARY ZLIB_LIBRARY ZSTD_LIBRARY SZ_INCLUDE_DIR
  )
  if(SZ_FOUND)
    set(SZ_INCLUDE_DIRS ${SZ_INCLUDE_DIR})
    set(SZ_LIBRARIES ${SZ_LIBRARY} ${ZLIB_LIBRARY} ${ZSTD_LIBRARY})
    if(SZ_FOUND AND NOT TARGET SZ::SZ)
      add_library(SZ::SZ UNKNOWN IMPORTED)
      set_target_properties(SZ::SZ PROPERTIES
        IMPORTED_LOCATION             "${SZ_LIBRARY}"
        INTERFACE_LINK_LIBRARIES      "${SZ_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${SZ_INCLUDE_DIR}"
      )
    endif()
  endif()
endif()
