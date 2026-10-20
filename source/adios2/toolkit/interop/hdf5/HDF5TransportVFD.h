/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_TOOLKIT_INTEROP_HDF5_HDF5TRANSPORTVFD_H_
#define ADIOS2_TOOLKIT_INTEROP_HDF5_HDF5TRANSPORTVFD_H_

#include <hdf5.h>

#include <memory>

namespace adios2
{
class Transport;
}

/**
 * Create an HDF5 file-access property list backed by an open ADIOS2 transport.
 * The VFD takes shared ownership of the transport and closes it with the HDF5
 * file. Only read-only HDF5 opens are supported.
 */
hid_t H5Pset_fapl_adios2_transport(const std::shared_ptr<adios2::Transport> &transport,
                                   haddr_t size);

#endif /* ADIOS2_TOOLKIT_INTEROP_HDF5_HDF5TRANSPORTVFD_H_ */
