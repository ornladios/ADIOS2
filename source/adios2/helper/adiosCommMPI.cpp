/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosCommMPI.cpp
 */

#include "adiosCommMPI.h"

#include <utility>

#include "adiosComm.h"

#include "adios2/common/ADIOSMPI.h"

namespace adios2
{
namespace helper
{

Comm CommFromMPI(MPI_Comm mpiComm)
{
    // Forward to the private API.
    return Comm::FromMPI(mpiComm);
}

MPI_Comm CommAsMPI(Comm const &comm)
{
    // Forward to the private API.
    return comm.AsMPI();
}

} // end namespace helper
} // end namespace adios2
