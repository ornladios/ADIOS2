# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#---------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#---------------------------------------------------------------------#

add_test(NAME Utils.SstConnTool.Help
  COMMAND ${MPIEXEC_COMMAND} $<TARGET_FILE:sst_conn_tool> --help
)
set_tests_properties(Utils.SstConnTool.Help PROPERTIES
  PASS_REGULAR_EXPRESSION "Usage:  sst_conn_tool")

add_test(NAME Utils.SstConnTool.ListenConnectConflict
  COMMAND ${MPIEXEC_COMMAND} $<TARGET_FILE:sst_conn_tool> --listen --connect
)
set_tests_properties(Utils.SstConnTool.ListenConnectConflict PROPERTIES
  PASS_REGULAR_EXPRESSION "Only one of --listen or --connect can be specified")

add_test(NAME Utils.SstConnTool.FileScreenConflict
  COMMAND ${MPIEXEC_COMMAND} $<TARGET_FILE:sst_conn_tool> --file --screen
)
set_tests_properties(Utils.SstConnTool.FileScreenConflict PROPERTIES
  PASS_REGULAR_EXPRESSION "Only one of --screen or --file can be specified")

add_test(NAME Utils.SstConnTool.UnknownOption
  COMMAND ${MPIEXEC_COMMAND} $<TARGET_FILE:sst_conn_tool> --bogus
)
set_tests_properties(Utils.SstConnTool.UnknownOption PROPERTIES
  PASS_REGULAR_EXPRESSION "Usage:  sst_conn_tool")

if(UNIX)
  add_test(NAME Utils.SstConnTool.Connect
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/connect.sh $<TARGET_FILE:sst_conn_tool>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  set_tests_properties(Utils.SstConnTool.Connect PROPERTIES
    TIMEOUT 60
    PASS_REGULAR_EXPRESSION "Connection success, all is well!")
endif()
