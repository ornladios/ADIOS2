# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

if(ADIOS2_HAVE_SZ)
  fortran_add_test_helper(BPWriteReadSZ2D MPI_ONLY)
  fortran_add_test_helper(BPWriteReadSZ3D MPI_ONLY)
endif()

if(ADIOS2_HAVE_ZFP)
  fortran_add_test_helper(BPWriteReadZfp2D MPI_ONLY)
  fortran_add_test_helper(BPWriteReadZfp2DRemove MPI_ONLY)
endif()
