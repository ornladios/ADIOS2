# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0


if(NOT IME_FOUND)
  if((NOT IME_ROOT) AND (NOT (ENV{IME_ROOT} STREQUAL "")))
    set(IME_ROOT "$ENV{IME_ROOT}")
  endif()
  if(IME_ROOT)
    set(IME_INCLUDE_OPTS HINTS ${IME_ROOT}/include NO_DEFAULT_PATHS)
    set(IME_LIBRARY_OPTS
      HINTS ${IME_ROOT}/lib ${IME_ROOT}/lib64
      NO_DEFAULT_PATHS
    )
  endif()

  find_path(IME_INCLUDE_DIR im_client_native2.h ${IME_INCLUDE_OPTS})
  find_library(IME_LIBRARY NAMES im_client ${IME_LIBRARY_OPTS})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(IME
    FOUND_VAR IME_FOUND
    REQUIRED_VARS IME_LIBRARY IME_INCLUDE_DIR
  )
  if(IME_FOUND)
    set(IME_INCLUDE_DIRS ${IME_INCLUDE_DIR})
    set(IME_LIBRARIES ${IME_LIBRARY})
    if(IME_FOUND AND NOT TARGET IME::IME)
      add_library(IME::IME UNKNOWN IMPORTED)
      set_target_properties(IME::IME PROPERTIES
        IMPORTED_LOCATION             "${IME_LIBRARY}"
        INTERFACE_LINK_LIBRARIES      "${IME_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${IME_INCLUDE_DIR}"
      )
    endif()
  endif()
endif()
