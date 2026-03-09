/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __RESTART_H__
#define __RESTART_H__

#include "gray-scott.h"
#include "settings.h"

#include <adios2.h>
#include <mpi.h>

void WriteCkpt(MPI_Comm comm, const int step, const Settings &settings, const GrayScott &sim,
               adios2::IO io);
int ReadRestart(MPI_Comm comm, const Settings &settings, GrayScott &sim, adios2::IO io);

#endif
