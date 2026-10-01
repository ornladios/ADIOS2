# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(SstParamFails MPI_ALLOW "" Engine.SST. "")
gtest_add_tests_helper(SstWriterFails MPI_ALLOW "" Engine.SST. "")

# GNU-compatible ELF linker wrapping lets us test transport failures without
# requiring an RDMA device or creating real UCX endpoints.
if(ADIOS2_SST_HAVE_UCX AND CMAKE_SYSTEM_NAME STREQUAL "Linux"
   AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_executable(Test.SST.UCXEndpoints TestUCXEndpoints.c)
  target_include_directories(Test.SST.UCXEndpoints PRIVATE
    $<TARGET_PROPERTY:sst,INCLUDE_DIRECTORIES>)
  target_link_libraries(Test.SST.UCXEndpoints PRIVATE
    ucx::ucx adios2::thirdparty::EVPath
    adios2::thirdparty::perfstubs-interface Threads::Threads)
  target_compile_options(Test.SST.UCXEndpoints PRIVATE
    -ffunction-sections -fdata-sections)
  target_link_options(Test.SST.UCXEndpoints PRIVATE
    -Wl,--gc-sections -Wl,--wrap=ucp_ep_create -Wl,--wrap=ucp_ep_rkey_unpack
    -Wl,--wrap=ucp_ep_close_nbx -Wl,--wrap=ucp_worker_destroy -Wl,--wrap=ucp_cleanup)
  add_test(NAME Engine.SST.UCXEndpoints COMMAND Test.SST.UCXEndpoints)
  set_tests_properties(Engine.SST.UCXEndpoints PROPERTIES
    LABELS "SST;UCX" TIMEOUT 30)
endif()
