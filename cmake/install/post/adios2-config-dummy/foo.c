/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef WITH_ADIOS2
#include <adios2_c.h>
#if ADIOS2_USE_MPI
#include <mpi.h>
#endif
#endif

#include "foo.h"

void foo(void)
{
#ifdef WITH_ADIOS2
#if ADIOS2_USE_MPI
    adios2_adios *ctx = adios2_init_mpi(MPI_COMM_WORLD);
#else
    adios2_adios *ctx = adios2_init_serial();
#endif
#endif
}
