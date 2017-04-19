/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS_MPI_H_
#define ADIOS_MPI_H_

#ifdef ADIOS_HAVE_MPI
#define OMPI_SKIP_MPICXX 1 // workaround for OpenMPI forcing C++ bindings
#include <mpi.h>
#undef OMPI_SKIP_MPICXX
#else
#include "mpidummy.h"
#endif

#endif /* ADIOS_MPI_H_ */
