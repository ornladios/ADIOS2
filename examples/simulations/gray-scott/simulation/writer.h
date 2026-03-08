/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef __WRITER_H__
#define __WRITER_H__

#include <adios2.h>
#include <mpi.h>

#include "gray-scott.h"
#include "settings.h"

class Writer
{
public:
    Writer(const Settings &settings, const GrayScott &sim, adios2::IO io);
    void open(const std::string &fname, bool append);
    void write(int step, const GrayScott &sim);
    void close();

protected:
    Settings settings;

    adios2::IO io;
    adios2::Engine writer;
    adios2::Variable<double> var_u;
    adios2::Variable<double> var_v;
    adios2::Variable<int> var_step;
};

#endif
