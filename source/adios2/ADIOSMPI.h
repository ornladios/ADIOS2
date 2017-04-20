/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS2_ADIOSMPI_H_
#define ADIOS2_ADIOSMPI_H_

#include "adios2/ADIOSConfig.h"

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#else
#include "adios2/mpidummy.h"
#endif

#endif /* ADIOS2_ADIOSMPI_H_ */
