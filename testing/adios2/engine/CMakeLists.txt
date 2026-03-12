# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(bp)
add_subdirectory(skeleton)
add_subdirectory(inline)
add_subdirectory(null)
add_subdirectory(nullcore)
add_subdirectory(time-series)

if(ADIOS2_HAVE_HDF5)
  add_subdirectory(hdf5)
endif()

if(ADIOS2_HAVE_DataMan)
  add_subdirectory(dataman)
endif()

if(ADIOS2_HAVE_MHS)
  add_subdirectory(mhs)
endif()

if(ADIOS2_HAVE_SST)
  add_subdirectory(sst)
endif()

if(ADIOS2_HAVE_MPI)
  add_subdirectory(ssc)
  add_subdirectory(common)
endif()

add_subdirectory(staging-common)
