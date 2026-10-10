# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

message_start_thirdparty()

add_library(xxhash_wrapper INTERFACE)

if(ADIOS2_USE_EXTERNAL_XXHASH)
  find_package(xxHash REQUIRED)
  target_link_libraries(xxhash_wrapper INTERFACE
    $<BUILD_INTERFACE:xxHash::xxhash>
  )
else()
  target_include_directories(xxhash_wrapper INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/xxhash_wrapper>
  )
endif()

install(TARGETS xxhash_wrapper EXPORT adios2Exports)

message_end_thirdparty()
