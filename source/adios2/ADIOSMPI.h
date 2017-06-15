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

#include <climits> //UXXX_MAX
#include <cstdint> //SIZE_MAX

#if SIZE_MAX == UINT_MAX
#define ADIOS2_MPI_SIZE_T MPI_UNSIGNED
#elif SIZE_MAX == ULONG_MAX
#define ADIOS2_MPI_SIZE_T MPI_UNSIGNED_LONG
#elif SIZE_MAX == ULLONG_MAX
#define ADIOS2_MPI_SIZE_T MPI_UNSIGNED_LONG_LONG
#else
#error "size_t could not be mapped to a MPI Type"
#endif

#endif /* ADIOS2_ADIOSMPI_H_ */
