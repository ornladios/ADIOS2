# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

if(ADIOS2_HAVE_MPI)
  add_subdirectory(gray-scott)
  add_subdirectory(gray-scott-struct)

  if(ADIOS2_HAVE_Kokkos)
    add_subdirectory(gray-scott-kokkos)
  endif()

  add_subdirectory(heatTransfer)
endif()

add_subdirectory(korteweg-de-vries)
add_subdirectory(lorenz_ode)
