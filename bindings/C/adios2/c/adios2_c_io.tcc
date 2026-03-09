/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_BINDINGS_C_ADIOS2_C_IO_TCC_
#define ADIOS2_BINDINGS_C_ADIOS2_C_IO_TCC_

#include "adios2_c_io.h"

#include "adios2/core/IO.h"

namespace
{
adios2::Mode adios2_ToOpenMode(const adios2_mode modeC)
{
    adios2::Mode mode = adios2::Mode::Undefined;
    switch (modeC)
    {

    case adios2_mode_write:
        mode = adios2::Mode::Write;
        break;

    case adios2_mode_read:
        mode = adios2::Mode::Read;
        break;

    case adios2_mode_append:
        mode = adios2::Mode::Append;
        break;

    case adios2_mode_readRandomAccess:
        mode = adios2::Mode::ReadRandomAccess;
        break;

    default:
        break;
    }
    return mode;
}
} // end anonymous namespace

#endif
