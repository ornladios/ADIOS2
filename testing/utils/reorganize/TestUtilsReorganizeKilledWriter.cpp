/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Write a few BP5 steps and leave the file as a killed writer would: all steps
 * complete, but still marked as being written (active flag set in md.idx).
 */

#include <fstream>
#include <iostream>

#include <adios2.h>

int main()
{
    const char *fname = "KilledWriter.bp";
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("Write");
    io.SetEngine("BP5");
    auto var = io.DefineVariable<double>("x");
    adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
    for (int step = 0; step < 3; ++step)
    {
        writer.BeginStep();
        writer.Put(var, (double)step);
        writer.EndStep();
    }
    writer.Close();

    // offset of activeFlag in the BP5 md.idx header
    std::fstream idx(std::string(fname) + "/md.idx",
                     std::ios::in | std::ios::out | std::ios::binary);
    if (!idx.is_open())
    {
        std::cerr << "Cannot open md.idx" << std::endl;
        return 1;
    }
    idx.seekp(39);
    const char active = 1;
    idx.write(&active, 1);
    return idx.good() ? 0 : 1;
}
