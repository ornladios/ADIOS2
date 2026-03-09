/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <adios2_c.h>

#if ADIOS2_USE_MPI
#error "ADIOS2_USE_MPI is true for source not using ADIOS2 MPI bindings"
#endif

#include <stdio.h>

int main(void)
{
    adios2_adios *adios = adios2_init_serial();
    if (!adios)
    {
        fprintf(stderr, "adios2_init_serial() failed\n");
        return 1;
    }
    adios2_finalize(adios);
    return 0;
}
