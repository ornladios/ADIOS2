/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS2_ADIOSMPICOMMONLY_H_
#define ADIOS2_ADIOSMPICOMMONLY_H_

#include "adios2/ADIOSConfig.h"

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#else
#ifdef __cplusplus

namespace adios2
{
namespace helper
{
namespace mpi
{
using MPI_Comm = int;
} // end namespace mpi
} // end namespace helper
} // end namespace adios2
namespace adios2
{
using helper::mpi::MPI_Comm;
}
#else
typedef int MPI_Comm;
#endif

#endif

#endif /* ADIOS2_ADIOSMPICOMMONLY_H_ */
