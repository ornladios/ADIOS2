#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# ADIOS Common Dashboard Script
#
# This script contains basic dashboard driver code common to all
# clients.
#
#   # Client maintainer: me@mydomain.net
#   set(CTEST_SITE "machine.site")
#   set(CTEST_BUILD_NAME "Platform-Compiler")
#   set(CTEST_CONFIGURATION_TYPE Debug)
#   set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
#   set(dashboard_cache "
#   QT_QMAKE_EXECUTABLE:FILEPATH=$ENV{HOME}/Qt/4.8.1-gcc-x64/bin/qmake
#   ")
#   include(${CTEST_SCRIPT_DIRECTORY}/common/adios_common.cmake)
#
# Then run a scheduled task (cron job) with a command line such as
#
#   ctest -S ~/Dashboards/Scripts/my_dashboard.cmake -V
#
# By default the source and build trees will be placed in the path
# "../My Tests/" relative to your script location.
#
# The following variables may be set before including this script
# to configure it:
#
#   dashboard_model           = Nightly | Experimental | Continuous
#   dashboard_disable_loop    = For continuous dashboards, disable loop.
#   dashboard_root_name       = Change name of "MyTests" directory
#   dashboard_source_name     = Name of source directory (openqube)
#   dashboard_binary_name     = Name of binary directory (openqube-build)
#   dashboard_cache           = Initial CMakeCache.txt file content
#   dashboard_do_coverage     = True to enable coverage (ex: gcov)
#   dashboard_do_memcheck     = True to enable memcheck (ex: valgrind)
#   dashboard_git_url         = Custom git clone url
#   dashboard_git_branch      = Custom remote branch to track
#   dashboard_git_crlf        = Value of core.autocrlf for repository
#   CTEST_UPDATE_COMMAND      = path to svn command-line client
#   CTEST_BUILD_FLAGS         = build tool arguments (ex: -j2)
#   CTEST_DASHBOARD_ROOT      = Where to put source and build trees
#   CTEST_TEST_TIMEOUT        = Per-test timeout length
#   CTEST_TEST_ARGS           = ctest_test args (ex: PARALLEL_LEVEL 4)
#   CMAKE_MAKE_PROGRAM        = Path to "make" tool to use
#
# For Makefile generators the script may be executed from an
# environment already configured to use the desired compilers.
# Alternatively the environment may be set at the top of the script:
#
#   set(ENV{CC}  /path/to/cc)   # C compiler
#   set(ENV{CXX} /path/to/cxx)  # C++ compiler
#   set(ENV{FC}  /path/to/fc)   # Fortran compiler (optional)
#   set(ENV{LD_LIBRARY_PATH} /path/to/vendor/lib) # (if necessary)

set(CTEST_PROJECT_NAME "ADIOS2")
set(CTEST_DROP_SITE "open.cdash.org")
set(dashboard_git_url "https://github.com/ornladios/ADIOS2.git")

if(NOT dashboard_root_name)
  set(dashboard_root_name "Builds/My Tests")
endif()
if(NOT dashboard_source_name)
  set(dashboard_source_name "ADIOS2")
endif()
if(NOT dashboard_model)
  set(dashboard_model Experimental)
endif()
include(${CMAKE_CURRENT_LIST_DIR}/universal.cmake)
