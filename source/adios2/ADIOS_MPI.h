/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS_MPI_H_
#define ADIOS_MPI_H_

#include "ADIOSConfig.h"

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#else
#include "mpidummy.h"
#endif

#endif /* ADIOS_MPI_H_ */
