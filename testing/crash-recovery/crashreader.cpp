/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Verifier for crash-recovery testing. Opens a (possibly truncated) BP5 file
 * and reads every step it can, verifying content against the writer's
 * deterministic pattern.
 *
 * Exit codes: 0 = recovered cleanly (prints RECOVERED <n>), 2 = data
 * mismatch (silent corruption!), 3 = clean ADIOS exception (acceptable),
 * signals = the bug class we hunt. Modes: ra (random access) | stream.
 */
#include <adios2.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static float pattern(size_t step, size_t rank, size_t i)
{
    return (float)(step * 1000003 + rank * 10007 + i * 7 + 1);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: crashreader <file.bp> ra|stream\n");
        return 1;
    }
    const char *fname = argv[1];
    bool ra = (strcmp(argv[2], "ra") == 0);
    size_t recovered = 0;
    try
    {
        adios2::ADIOS adios;
        adios2::IO io = adios.DeclareIO("r");
        io.SetEngine("BP5");
        if (const char *w = getenv("CR_EOF_WAIT"))
            io.SetParameter("EOFWaitSecs", w);
        if (!ra)
        {
            // bound the streaming Open: a writer killed before producing a
            // usable header would otherwise block Open indefinitely
            io.SetParameter("OpenTimeoutSecs", "2");
        }
        adios2::Engine r = io.Open(fname, ra ? adios2::Mode::ReadRandomAccess : adios2::Mode::Read);
        if (ra)
        {
            auto va = io.InquireVariable<float>("a");
            if (!va)
            {
                printf("RECOVERED 0 (no variable)\n");
                r.Close();
                return 0;
            }
            const size_t nsteps = va.Steps();
            const size_t n = va.Shape()[0];
            std::vector<float> buf(n);
            for (size_t s = 0; s < nsteps; s++)
            {
                va.SetStepSelection({s, 1});
                r.Get(va, buf.data(), adios2::Mode::Sync);
                for (size_t i = 0; i < n; i++)
                    if (buf[i] != pattern(s, 0, i))
                    {
                        printf("MISMATCH step %zu elem %zu: %g vs %g\n", s, i, buf[i],
                               pattern(s, 0, i));
                        return 2;
                    }
                recovered++;
            }
            r.Close();
        }
        else
        {
            // bounded BeginStep: a dead writer's file never gets EndOfStream,
            // so a correct application polls with a timeout and gives up
            adios2::StepStatus st;
            while ((st = r.BeginStep(adios2::StepMode::Read, 1.0f)) !=
                   adios2::StepStatus::EndOfStream)
            {
                if (st == adios2::StepStatus::NotReady)
                {
                    printf("GAVE_UP (NotReady) after %zu steps\n", recovered);
                    break;
                }
                auto va = io.InquireVariable<float>("a");
                auto vs = io.InquireVariable<uint64_t>("stepnum");
                if (!va || !vs)
                {
                    r.EndStep();
                    continue;
                }
                const size_t n = va.Shape()[0];
                std::vector<float> buf(n);
                uint64_t stepnum = ~0ull;
                r.Get(va, buf.data());
                r.Get(vs, &stepnum);
                r.EndStep();
                for (size_t i = 0; i < n; i++)
                    if (buf[i] != pattern(stepnum, 0, i))
                    {
                        printf("MISMATCH step %llu elem %zu: %g vs %g\n",
                               (unsigned long long)stepnum, i, buf[i], pattern(stepnum, 0, i));
                        return 2;
                    }
                recovered++;
            }
            r.Close();
        }
    }
    catch (std::exception &e)
    {
        printf("CLEAN_ERROR after %zu steps: %s\n", recovered, e.what());
        return 3;
    }
    printf("RECOVERED %zu\n", recovered);
    return 0;
}
