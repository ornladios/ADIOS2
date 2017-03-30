#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# "Universal" Dashboard Script
#
# This script contains basic dashboard driver code universal to all
# clients *and* all projects.
#
# This script is in "common" in the DashboardScriptsNG repo.
#
# Create a project-specific common script with code of the following form,
# where the final line includes this script.
#
#   set(CTEST_PROJECT_NAME "OpenChemistry")
#   set(CTEST_DROP_SITE "cdash.openchemistry.org")
#
#   set(dashboard_git_url "git://source.openchemistry.org/openchemistry.git")
#   set(dashboard_root_name "MyTests")
#   set(dashboard_source_name "openchemistry")
#
#   get_filename_component(dir ${CMAKE_CURRENT_LIST_FILE} PATH)
#   include(${dir}/universal.cmake)
#

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

if(NOT DEFINED CTEST_PROJECT_NAME)
  message(FATAL_ERROR "project-specific script including 'universal.cmake' should set CTEST_PROJECT_NAME")
endif()

if(NOT DEFINED dashboard_user_home)
  set(dashboard_user_home "$ENV{HOME}")
endif()

# Select the top dashboard directory.
if(NOT DEFINED dashboard_root_name)
  set(dashboard_root_name "MyTests")
endif()
if(NOT DEFINED CTEST_DASHBOARD_ROOT)
  get_filename_component(CTEST_DASHBOARD_ROOT "${CTEST_SCRIPT_DIRECTORY}/../${dashboard_root_name}" ABSOLUTE)
endif()

# Select the model (Nightly, Experimental, Continuous).
if(NOT DEFINED dashboard_model)
  set(dashboard_model Nightly)
endif()
if(NOT "${dashboard_model}" MATCHES "^(Nightly|Experimental|Continuous)$")
  message(FATAL_ERROR "dashboard_model must be Nightly, Experimental, or Continuous")
endif()

# Default to a Debug build.
if(NOT DEFINED CTEST_CONFIGURATION_TYPE AND DEFINED CTEST_BUILD_CONFIGURATION)
  set(CTEST_CONFIGURATION_TYPE ${CTEST_BUILD_CONFIGURATION})
endif()

if(NOT DEFINED CTEST_CONFIGURATION_TYPE)
  set(CTEST_CONFIGURATION_TYPE Debug)
endif()

# Choose CTest reporting mode.
if(NOT "${CTEST_CMAKE_GENERATOR}" MATCHES "Make")
  # Launchers work only with Makefile generators.
  set(CTEST_USE_LAUNCHERS 0)
elseif(NOT DEFINED CTEST_USE_LAUNCHERS)
  # The setting is ignored by CTest < 2.8 so we need no version test.
  set(CTEST_USE_LAUNCHERS 1)
endif()

# Configure testing.
if(NOT DEFINED CTEST_TEST_CTEST)
  set(CTEST_TEST_CTEST 1)
endif()
if(NOT CTEST_TEST_TIMEOUT)
  set(CTEST_TEST_TIMEOUT 1500)
endif()

# Select Git source to use.
if(NOT DEFINED dashboard_git_url)
  message(FATAL_ERROR "project-specific script including 'universal.cmake' should set dashboard_git_url")
endif()

if(NOT DEFINED dashboard_git_branch)
  if("${dashboard_model}" STREQUAL "Nightly")
    set(dashboard_git_branch nightly-master )
  else()
    set(dashboard_git_branch master)
  endif()
endif()
if(NOT DEFINED dashboard_git_crlf)
  if(UNIX)
    set(dashboard_git_crlf false)
  else()
    set(dashboard_git_crlf true)
  endif()
endif()

# Look for a GIT command-line client.
if(NOT DEFINED CTEST_GIT_COMMAND)
  find_program(CTEST_GIT_COMMAND NAMES git git.exe git.cmd)
endif()

if(NOT EXISTS ${CTEST_GIT_COMMAND})
  message(FATAL_ERROR "No Git Found.")
endif()

# Select a source directory name.
if(NOT DEFINED CTEST_SOURCE_DIRECTORY)
  if(DEFINED dashboard_source_name)
    set(CTEST_SOURCE_DIRECTORY ${CTEST_DASHBOARD_ROOT}/${dashboard_source_name})
  else()
    set(CTEST_SOURCE_DIRECTORY ${CTEST_DASHBOARD_ROOT}/${CTEST_PROJECT_NAME})
  endif()
endif()

# Select a build directory name.
if(NOT DEFINED CTEST_BINARY_DIRECTORY)
  if(DEFINED dashboard_binary_name)
    set(CTEST_BINARY_DIRECTORY ${CTEST_DASHBOARD_ROOT}/${dashboard_binary_name})
  else()
    set(CTEST_BINARY_DIRECTORY ${CTEST_SOURCE_DIRECTORY}-build)
  endif()
endif()

# Support initial checkout if necessary.
if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}" AND NOT DEFINED CTEST_CHECKOUT_COMMAND)
  get_filename_component(_name "${CTEST_SOURCE_DIRECTORY}" NAME)
  if(CTEST_GIT_COMMAND)
    execute_process(COMMAND ${CTEST_GIT_COMMAND} --version OUTPUT_VARIABLE output)
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+(\\.g[0-9a-f]+)?)?" GIT_VERSION "${output}")
    if(NOT "${GIT_VERSION}" VERSION_LESS "1.6.5")
      # Have "git clone -b <branch>" option.
      set(git_branch_new "-b ${dashboard_git_branch}")
      set(git_branch_old)
    else()
      # No "git clone -b <branch>" option.
      set(git_branch_new)
      set(git_branch_old "-b ${dashboard_git_branch} origin/${dashboard_git_branch}")
    endif()

    # Generate an initial checkout script.
    set(ctest_checkout_script ${CTEST_DASHBOARD_ROOT}/${_name}-init.cmake)
    file(WRITE ${ctest_checkout_script} "# git repo init script for ${_name}
execute_process(
  COMMAND \"${CTEST_GIT_COMMAND}\" clone -n ${git_branch_new} \"${dashboard_git_url}\"
          \"${CTEST_SOURCE_DIRECTORY}\"
  )
if(EXISTS \"${CTEST_SOURCE_DIRECTORY}/.git\")
  execute_process(
    COMMAND \"${CTEST_GIT_COMMAND}\" config core.autocrlf ${dashboard_git_crlf}
    WORKING_DIRECTORY \"${CTEST_SOURCE_DIRECTORY}\"
    )
  execute_process(
    COMMAND \"${CTEST_GIT_COMMAND}\" checkout ${git_branch_old}
    WORKING_DIRECTORY \"${CTEST_SOURCE_DIRECTORY}\"
    )
  execute_process(
    COMMAND \"${CTEST_GIT_COMMAND}\" submodule init
    WORKING_DIRECTORY \"${CTEST_SOURCE_DIRECTORY}\"
    )
  execute_process(
    COMMAND \"${CTEST_GIT_COMMAND}\" submodule update
    WORKING_DIRECTORY \"${CTEST_SOURCE_DIRECTORY}\"
    )
endif()
")
    set(CTEST_CHECKOUT_COMMAND "\"${CMAKE_COMMAND}\" -P \"${ctest_checkout_script}\"")
  endif()

  # CTest delayed initialization is broken, so we put the
  # CTestConfig.cmake info here.
  if(NOT DEFINED CTEST_NIGHTLY_START_TIME)
    set(CTEST_NIGHTLY_START_TIME "01:00:00 UTC")
  endif()
  if(NOT DEFINED CTEST_DROP_METHOD)
    set(CTEST_DROP_METHOD "http")
  endif()
  if(NOT DEFINED CTEST_DROP_SITE)
    set(CTEST_DROP_SITE "open.cdash.org")
  endif()
  if(NOT DEFINED CTEST_DROP_LOCATION)
    set(CTEST_DROP_LOCATION "/submit.php?project=${CTEST_PROJECT_NAME}")
  endif()
  if(NOT DEFINED CTEST_DROP_SITE_CDASH)
    set(CTEST_DROP_SITE_CDASH TRUE)
  endif()
endif()

#-----------------------------------------------------------------------------

# Send the main script as a note.
list(APPEND CTEST_NOTES_FILES
  "${CTEST_SCRIPT_DIRECTORY}/${CTEST_SCRIPT_NAME}"
  "${CMAKE_CURRENT_LIST_FILE}"
  )

# Check for required variables.
foreach(req
    CTEST_CMAKE_GENERATOR
    CTEST_SITE
    CTEST_BUILD_NAME
    )
  if(NOT DEFINED ${req})
    message(FATAL_ERROR "The containing script must set ${req}")
  endif()
endforeach(req)

# Print summary information.
foreach(v
    CTEST_SITE
    CTEST_BUILD_NAME
    CTEST_SOURCE_DIRECTORY
    CTEST_BINARY_DIRECTORY
    CTEST_CMAKE_GENERATOR
    CTEST_CONFIGURATION_TYPE
    CTEST_GIT_COMMAND
    CTEST_CHECKOUT_COMMAND
    CTEST_SCRIPT_DIRECTORY
    CTEST_USE_LAUNCHERS
    )
  set(vars "${vars}  ${v}=[${${v}}]\n")
endforeach(v)
message("Dashboard script configuration:\n${vars}\n")

# Avoid non-ascii characters in tool output.
set(ENV{LC_ALL} C)

# Helper macro to write the initial cache.
macro(write_cache)
  set(cache_build_type "")
  set(cache_make_program "")
  if(CTEST_CMAKE_GENERATOR MATCHES "Make")
    set(cache_build_type CMAKE_BUILD_TYPE:STRING=${CTEST_CONFIGURATION_TYPE})
    if(CMAKE_MAKE_PROGRAM)
      set(cache_make_program CMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM})
    endif()
  endif()
  file(WRITE ${CTEST_BINARY_DIRECTORY}/CMakeCache.txt "
SITE:STRING=${CTEST_SITE}
BUILDNAME:STRING=${CTEST_BUILD_NAME}
CTEST_USE_LAUNCHERS:BOOL=${CTEST_USE_LAUNCHERS}
DART_TESTING_TIMEOUT:STRING=${CTEST_TEST_TIMEOUT}
${cache_build_type}
${cache_make_program}
${dashboard_cache}
")
endmacro(write_cache)

# Start with a fresh build tree.
file(MAKE_DIRECTORY "${CTEST_BINARY_DIRECTORY}")
if(NOT "${CTEST_SOURCE_DIRECTORY}" STREQUAL "${CTEST_BINARY_DIRECTORY}")
  message("Clearing build tree...")
  ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})
endif()

set(dashboard_continuous 0)
if("${dashboard_model}" STREQUAL "Continuous")
  set(dashboard_continuous 1)
endif()

# CTest 2.6 crashes with message() after ctest_test.
macro(safe_message)
  if(NOT "${CMAKE_VERSION}" VERSION_LESS 2.8 OR NOT safe_message_skip)
    message(${ARGN})
  endif()
endmacro()

function(upload_to_midas upload_file)
  get_filename_component(_self_dir ${CMAKE_CURRENT_LIST_FILE} PATH)
  execute_process(COMMAND ${CMAKE_COMMAND}
    -D midas_key_file:STRING=${midas_key_file}
    -D midas_product_name:STRING=${midas_product_name}
    -D midas_folder_id:STRING=${midas_folder_id}
    -D midas_app_id:STRING=${midas_app_id}
    -D midas_submission_type:STRING=${dashboard_model}
    -D midas_file:STRING=${upload_file}
    -P ${_self_dir}/UploadToMidas.cmake
    RESULT_VARIABLE rv)
  if(NOT "${rv}" STREQUAL "0")
    message(SEND_ERROR "error calling UploadToMidas.cmake rv='${rv}'")
  endif()
endfunction()

function(upload_package_files script_file)
  if(EXISTS "${script_file}")
    include("${script_file}")
    set(regex "\\.tar\\.gz$")
    if(WIN32)
      set(regex "\\.exe$")
    elseif(APPLE)
      set(regex "\\.dmg$")
    endif()
    # script_file should have a "set(package_files" in it...
    foreach(f ${package_files})
      if("${f}" MATCHES "${regex}")
        safe_message("uploading file '${f}' to CDash")
        ctest_upload(FILES "${f}")
        ctest_submit(PARTS Upload)
        if(DEFINED midas_product_name)
          safe_message("uploading file '${f}' to Midas")
          upload_to_midas(${f})
        endif()
      endif()
    endforeach()
  endif()
endfunction()

if(COMMAND dashboard_hook_init)
  dashboard_hook_init()
endif()

set(dashboard_done 0)
while(NOT dashboard_done)
  if(dashboard_continuous)
    set(START_TIME ${CTEST_ELAPSED_TIME})
  endif()
  set(ENV{HOME} "${dashboard_user_home}")

  # Start a new submission.
  if(COMMAND dashboard_hook_start)
    dashboard_hook_start()
  endif()
  safe_message("Calling ctest_start...")
  ctest_start(${dashboard_model})

  # Always build if the tree is fresh.
  set(dashboard_fresh 0)
  if(NOT EXISTS "${CTEST_BINARY_DIRECTORY}/CMakeCache.txt")
    set(dashboard_fresh 1)
    safe_message("Starting fresh build...")
    write_cache()
  endif()

  # Look for updates.
  safe_message("Calling ctest_update...")
  ctest_update(RETURN_VALUE count)
  set(CTEST_CHECKOUT_COMMAND) # checkout on first iteration only
  safe_message("Found ${count} changed files")

  if(dashboard_fresh OR NOT dashboard_continuous OR count GREATER 0)
    execute_process(
      COMMAND ${CTEST_GIT_COMMAND} submodule init
      WORKING_DIRECTORY ${CTEST_SOURCE_DIRECTORY}
      )
    execute_process(
      COMMAND ${CTEST_GIT_COMMAND} submodule sync
      WORKING_DIRECTORY ${CTEST_SOURCE_DIRECTORY}
      )
    execute_process(
      COMMAND ${CTEST_GIT_COMMAND} submodule update
      WORKING_DIRECTORY ${CTEST_SOURCE_DIRECTORY}
      )

    safe_message("Calling ctest_configure...")
    ctest_configure()
    ctest_submit(PARTS Update Configure Notes)
    ctest_read_custom_files(${CTEST_BINARY_DIRECTORY})

    if(COMMAND dashboard_hook_build)
      dashboard_hook_build()
    endif()
    safe_message("Calling ctest_build...")
    ctest_build(APPEND)
    ctest_submit(PARTS Build)

    if(COMMAND dashboard_hook_test)
      dashboard_hook_test()
    endif()
    safe_message("Calling ctest_test...")
    ctest_test(${CTEST_TEST_ARGS} APPEND)
    ctest_submit(PARTS Test)
    set(safe_message_skip 1) # Block furhter messages

    upload_package_files("${CTEST_BINARY_DIRECTORY}/BuildPackageTestResults.cmake")

    if(dashboard_do_coverage)
      safe_message("Calling ctest_coverage...")
      ctest_coverage()
      ctest_submit(PARTS Coverage)
    endif()
    if(dashboard_do_memcheck)
      safe_message("Calling ctest_memcheck...")
      ctest_memcheck()
      ctest_submit(PARTS MemCheck)
    endif()
    if(COMMAND dashboard_hook_submit)
      dashboard_hook_submit()
    endif()
    if(COMMAND dashboard_hook_end)
      dashboard_hook_end()
    endif()
  endif()

  if(dashboard_continuous AND NOT dashboard_disable_loop)
    safe_message("Delay before looping for Continuous...")
    # Delay until at least 5 minutes past START_TIME
    ctest_sleep(${START_TIME} 300 ${CTEST_ELAPSED_TIME})
    if(${CTEST_ELAPSED_TIME} GREATER 57600)
      set(dashboard_done 1)
    endif()
  else()
    # Not continuous, so we are done.
    set(dashboard_done 1)
  endif()
endwhile()
