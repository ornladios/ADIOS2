# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  set(MPIEXEC_COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS})
  add_subdirectory(iotest)
else()
  set(MPIEXEC_COMMAND)
endif()

if(ADIOS2_HAVE_Python)
  add_subdirectory(bpcmp)
endif()

add_subdirectory(cwriter)
add_subdirectory(changingshape)

