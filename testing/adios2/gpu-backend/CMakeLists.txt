# ------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
# ------------------------------------------------------------------------------#
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(DEFINED Kokkos_CXX_COMPILER)
  set(CMAKE_CXX_COMPILER "${Kokkos_CXX_COMPILER}")
endif()

gtest_add_tests_helper(WriteReadKokkos MPI_ALLOW BP Engine.BP. .BP5
  WORKING_DIRECTORY ${BP5_DIR} EXTRA_ARGS "BP5"
)

foreach(tgt ${Test.Engine.BP.WriteReadKokkos-TARGETS})
  target_link_libraries(${tgt} Kokkos::kokkos)
endforeach()
