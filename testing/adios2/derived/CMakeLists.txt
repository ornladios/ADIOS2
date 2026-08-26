# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
#Distributed under the OSI - approved Apache License, Version 2.0. See
#accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(DerivedCorrectness MPI_NONE BP Derived. "")
gtest_add_tests_helper(DerivedCorrectnessMPI MPI_ALLOW BP Derived. "")
gtest_add_tests_helper(ReaderDerived MPI_NONE BP Derived. "")
gtest_add_tests_helper(ReaderDerivedMPI MPI_ONLY BP Derived. "")

add_executable(Test.Derived.Fusion.Serial TestDerivedFusion.cpp)
target_link_libraries(Test.Derived.Fusion.Serial adios2_core adios2_core_derived adios2::thirdparty::gtest)
set_target_properties(Test.Derived.Fusion.Serial PROPERTIES
  INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${ADIOS2_SOURCE_DIR}/source>;$<BUILD_INTERFACE:${ADIOS2_SOURCE_DIR}/source/adios2/toolkit/derived/parser>;$<BUILD_INTERFACE:${ADIOS2_BINARY_DIR}/source/adios2>")
add_test(NAME Derived.Fusion.Serial COMMAND Test.Derived.Fusion.Serial)

add_executable(Bench.Derived.Fused BenchFused.cpp)
target_link_libraries(Bench.Derived.Fused adios2_core adios2_core_derived)
set_target_properties(Bench.Derived.Fused PROPERTIES
  INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${ADIOS2_SOURCE_DIR}/source>;$<BUILD_INTERFACE:${ADIOS2_SOURCE_DIR}/source/adios2/toolkit/derived/parser>;$<BUILD_INTERFACE:${ADIOS2_BINARY_DIR}/source/adios2>")
