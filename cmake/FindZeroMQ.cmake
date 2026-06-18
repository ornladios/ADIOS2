# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

if(ZeroMQ_FOUND)
  return()
endif()

# Use find_package instead of find_dependency because we do NOT want PkgConfig to be REQUIRED.
# If PkgConfig is missing, we simply stop without producing a fatal error.
find_package(PkgConfig QUIET)
if(NOT PkgConfig_FOUND)
    return()
endif()

pkg_check_modules(libzmq IMPORTED_TARGET libzmq)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  ZeroMQ
  REQUIRED_VARS libzmq_FOUND
  VERSION_VAR libzmq_VERSION
)
# Create alias target to match imported target name of the native package
# Only create the alias if ZeroMQ was successfully found.
# This prevents creating an alias when version checks failed or the module is incomplete.
if(ZeroMQ_FOUND AND TARGET PkgConfig::libzmq)
  add_library(libzmq ALIAS PkgConfig::libzmq)
endif()
