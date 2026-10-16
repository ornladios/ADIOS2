# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

include(ADIOSFunctions)

add_executable(Test.Utils.Reorganize.KilledWriter TestUtilsReorganizeKilledWriter.cpp)
target_link_libraries(Test.Utils.Reorganize.KilledWriter adios2::cxx)

# a file left active by a killed writer is read to its end with --ignore-active-flag
add_test(NAME Utils.Reorganize.KilledWriter
  COMMAND $<TARGET_FILE:Test.Utils.Reorganize.KilledWriter>
)
add_test(NAME Utils.Reorganize.KilledWriter.Reorganize
  COMMAND $<TARGET_FILE:adios_reorganize> --ignore-active-flag
    KilledWriter.bp KilledWriterOut.bp BP5 "" BP5 ""
)
set_tests_properties(Utils.Reorganize.KilledWriter.Reorganize PROPERTIES
  PASS_REGULAR_EXPRESSION "Bye after processing 3 steps"
  TIMEOUT 60
)
SetupTestPipeline(Utils.Reorganize.KilledWriter ";Reorganize" TRUE)
