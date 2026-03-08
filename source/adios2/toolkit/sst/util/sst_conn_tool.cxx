/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sst_comm_fwd.h"

#if ADIOS2_USE_MPI
#include "adios2/helper/adiosCommMPI.h"
static adios2::helper::Comm CommWorld = adios2::helper::CommWithMPI(MPI_COMM_WORLD);
#else
#include "adios2/helper/adiosCommDummy.h"
static adios2::helper::Comm CommWorld = adios2::helper::CommDummy();
#endif

extern "C" {
SMPI_Comm SMPI_COMM_WORLD = &CommWorld;

int SMPI_Init(int *argc, char ***argv)
{
#if ADIOS2_USE_MPI
    return MPI_Init(argc, argv);
#else
    static_cast<void>(argc);
    static_cast<void>(argv);
    return 0;
#endif
}
}
