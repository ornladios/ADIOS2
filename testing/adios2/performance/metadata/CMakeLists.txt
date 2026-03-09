# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  # just for executing manually for performance studies
  add_executable(PerfMetaData PerfMetaData.cpp)
  target_link_libraries(PerfMetaData adios2::cxx_mpi MPI::MPI_CXX)
endif()
