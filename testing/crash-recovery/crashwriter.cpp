/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Writer for crash-recovery testing: writes deterministic per-step data so a
 * reader can verify integrity of whatever survives. Designed to be killed.
 */
#include <adios2.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

// data[i] at (step, rank) = pattern(step, rank, i): recomputable by the reader
static float pattern(size_t step, size_t rank, size_t i)
{
    return (float)(step * 1000003 + rank * 10007 + i * 7 + 1);
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        fprintf(stderr, "usage: crashwriter <file.bp> <steps> <elems> [sleep_ms_per_step]\n");
        return 1;
    }
    const char *fname = argv[1];
    size_t steps = strtoul(argv[2], nullptr, 10);
    size_t n = strtoul(argv[3], nullptr, 10);
    int sleep_ms = argc > 4 ? atoi(argv[4]) : 0;

    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("w");
    io.SetEngine("BP5");
    auto va = io.DefineVariable<float>("a", {n}, {0}, {n});
    auto vstep = io.DefineVariable<uint64_t>("stepnum");
    io.DefineAttribute<int>("meaning", 42);
    adios2::Engine w = io.Open(fname, adios2::Mode::Write);
    std::vector<float> buf(n);
    for (size_t s = 0; s < steps; s++)
    {
        w.BeginStep();
        for (size_t i = 0; i < n; i++)
            buf[i] = pattern(s, 0, i);
        w.Put(va, buf.data());
        w.Put(vstep, (uint64_t)s);
        w.EndStep();
        // stdout marker: harness learns how many EndSteps definitely completed
        printf("ENDSTEP %zu\n", s);
        fflush(stdout);
        if (sleep_ms)
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
    w.Close();
    printf("CLOSED\n");
    return 0;
}
