/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include <adios2_c.h>

#include <stdio.h>

int main(void)
{
    adios2_adios *adios = adios2_init(adios2_debug_mode_on);
    if (!adios)
    {
        fprintf(stderr, "adios2_init() failed\n");
        return 1;
    }
    adios2_finalize(adios);
    return 0;
}
