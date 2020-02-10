/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include "mpiwrap.h"

SMPI_Comm SMPI_COMM_WORLD = MPI_COMM_WORLD;

extern "C" {
int SMPI_Init(int *argc, char ***argv)
{
#ifdef ADIOS2_HAVE_MPI
    return MPI_Init(argc, argv);
#else
    static_cast<void>(argc);
    static_cast<void>(argv);
    return 0;
#endif
}
}
