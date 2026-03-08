# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(adios2_campaign_write_mpi campaign_write.cpp)
  target_link_libraries(adios2_campaign_write_mpi adios2::cxx_mpi adios2_core MPI::MPI_CXX)
endif()
