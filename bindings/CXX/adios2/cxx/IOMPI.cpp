/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IOMPI.cpp :
 */

#include "IO.h"

#include "adios2/core/IO.h"

#include "adios2/helper/adiosCommMPI.h"

namespace adios2
{

Engine IO::Open(const std::string &name, const Mode mode, MPI_Comm comm)
{
    helper::CheckForNullptr(m_IO, "for engine " + name + ", in call to IO::Open");
    return Engine(&m_IO->Open(name, mode, helper::CommDupMPI(comm)));
}

} // end namespace adios2
