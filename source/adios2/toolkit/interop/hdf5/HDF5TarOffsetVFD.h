/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*
  A minimal offset VFD written by codex to read from a HDF5 file sitting inside a TAR file at
  'physical_offset'. HDF5 thinks it is reading a normal HDF5 file starting at address 0, but the
  driver translates every read to: physical_offset = tar_member_offset_data + hdf5_logical_addr
*/
#include <hdf5.h>

/**
 * offset: location of HDF5 file in a TAR file
 * size: size of the HDF5 file
 */
hid_t H5Pset_fapl_taroffset(haddr_t offset, haddr_t size);