/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_OPENFILE_H_
#define ADIOS2_TOOLKIT_TRANSPORT_OPENFILE_H_

#include <memory>
#include <string>

#include "adios2/common/ADIOSTypes.h"
#include "adios2/toolkit/transport/Transport.h"

namespace adios2
{
namespace helper
{
class Comm;
} // end namespace helper
namespace transport
{

/**
 * Open a file transport, choosing the underlying library from parameters
 * (POSIX, stdio, fstream, DAOS, etc.). The transport's Open() is called
 * with the supplied comm.
 */
std::shared_ptr<Transport> OpenFile(helper::Comm const &comm, std::string const &name, Mode mode,
                                    Params const &parameters, bool profile);

/**
 * Like OpenFile, but the transport's OpenChain() is called with chainComm
 * to avoid simultaneous opens (DOS-style) on the filesystem.
 */
std::shared_ptr<Transport> OpenFileChained(helper::Comm const &comm, std::string const &name,
                                           Mode mode, Params const &parameters, bool profile,
                                           helper::Comm const &chainComm);

} // end namespace transport
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_TRANSPORT_OPENFILE_H_ */
