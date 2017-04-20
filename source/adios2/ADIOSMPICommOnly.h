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
namespace adios
{
    using MPI_Comm = int;
} // end namespace adios
#endif

#endif /* ADIOS2_ADIOSMPICOMMONLY_H_ */
