/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// BP5 GetContext isolation and concurrent-use tests.  Local BP5 only.

#include <array>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

#include <adios2.h>
#include <adios2/core/GetContext.h>

#include <gtest/gtest.h>

#include "../TestHelpers.h"

std::string engineName;       // from command line
std::string engineParameters; // from command line

class BPGetContextIsolation : public ::testing::Test
{
public:
    BPGetContextIsolation() = default;
};

TEST_F(BPGetContextIsolation, TwoIndependentContexts)
{
    const size_t Nx = 8;
    const std::string fname("BPGetContextIsolation.bp");

    adios2::ADIOS adios;

    {
        adios2::IO io = adios.DeclareIO("WriteIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }

        auto v0 = io.DefineVariable<std::int32_t>("v0", {Nx}, {0}, {Nx});
        auto v1 = io.DefineVariable<double>("v1", {Nx}, {0}, {Nx});

        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        std::vector<std::int32_t> v0_data(Nx);
        std::vector<double> v1_data(Nx);
        for (size_t i = 0; i < Nx; ++i)
        {
            v0_data[i] = static_cast<std::int32_t>(100 + i);
            v1_data[i] = 0.5 + static_cast<double>(i);
        }
        writer.BeginStep();
        writer.Put(v0, v0_data.data());
        writer.Put(v1, v1_data.data());
        writer.EndStep();
        writer.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);

        auto v0 = io.InquireVariable<std::int32_t>("v0");
        auto v1 = io.InquireVariable<double>("v1");
        ASSERT_TRUE(v0);
        ASSERT_TRUE(v1);

        auto ctxA = reader.NewGetContext();
        auto ctxB = reader.NewGetContext();
        ASSERT_NE(ctxA, nullptr);
        ASSERT_NE(ctxB, nullptr);
        ASSERT_NE(ctxA.get(), ctxB.get());

        constexpr std::int32_t I_SENTINEL = -1;
        constexpr double D_SENTINEL = -1.0;
        std::vector<std::int32_t> bufA(Nx, I_SENTINEL);
        std::vector<double> bufB(Nx, D_SENTINEL);

        const auto sel = adios2::Selection::All().WithSteps(0, 1);

        reader.Get(*ctxA, v0, bufA.data(), sel);
        reader.Get(*ctxB, v1, bufB.data(), sel);

        // PerformGets(ctxA) must fill bufA only, leave bufB untouched.
        reader.PerformGets(*ctxA);
        for (size_t i = 0; i < Nx; ++i)
        {
            EXPECT_EQ(bufA[i], static_cast<std::int32_t>(100 + i))
                << "bufA[" << i << "] not filled by PerformGets(ctxA)";
            EXPECT_DOUBLE_EQ(bufB[i], D_SENTINEL)
                << "bufB[" << i << "] = " << bufB[i]
                << " was modified by PerformGets(ctxA) — context isolation violated";
        }

        reader.PerformGets(*ctxB);
        for (size_t i = 0; i < Nx; ++i)
        {
            EXPECT_DOUBLE_EQ(bufB[i], 0.5 + static_cast<double>(i))
                << "bufB[" << i << "] not filled by PerformGets(ctxB)";
        }

        // Reuse ctxA after PerformGets — queue must have been cleared.
        std::vector<std::int32_t> bufA2(Nx, I_SENTINEL);
        reader.Get(*ctxA, v0, bufA2.data(), sel);
        reader.PerformGets(*ctxA);
        for (size_t i = 0; i < Nx; ++i)
        {
            EXPECT_EQ(bufA2[i], static_cast<std::int32_t>(100 + i))
                << "bufA2[" << i << "] — ctxA reuse after PerformGets failed";
        }

        // Empty-queue PerformGets must be a benign no-op.
        EXPECT_NO_THROW(reader.PerformGets(*ctxA));

        reader.Close();
    }

#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
}

TEST_F(BPGetContextIsolation, ConcurrentGetsOnSameEngine)
{
    constexpr size_t Nx = 1024;
    constexpr size_t NumVars = 4;
    constexpr size_t NumThreads = NumVars;
    const std::string fname("BPGetContextConcurrent.bp");

    adios2::ADIOS adios;

    {
        adios2::IO io = adios.DeclareIO("ConcWriteIO");
        if (!engineName.empty())
            io.SetEngine(engineName);
        if (!engineParameters.empty())
            io.SetParameters(engineParameters);

        std::array<adios2::Variable<double>, NumVars> vars;
        for (size_t v = 0; v < NumVars; ++v)
        {
            vars[v] = io.DefineVariable<double>("var" + std::to_string(v), {Nx}, {0}, {Nx});
        }

        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        for (int step = 0; step < 3; ++step)
        {
            writer.BeginStep();
            for (size_t v = 0; v < NumVars; ++v)
            {
                std::vector<double> data(Nx);
                for (size_t i = 0; i < Nx; ++i)
                    data[i] = static_cast<double>(step * 1000 + v * 100 + i);
                writer.Put(vars[v], data.data());
            }
            writer.EndStep();
        }
        writer.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ConcReadIO");
        if (!engineName.empty())
            io.SetEngine(engineName);
        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);

        std::vector<std::string> errors(NumThreads);
        std::vector<std::thread> threads;

        for (size_t t = 0; t < NumThreads; ++t)
        {
            threads.emplace_back(
                [&](size_t tid) {
                    try
                    {
                        auto var = io.InquireVariable<double>("var" + std::to_string(tid));
                        if (!var)
                        {
                            errors[tid] = "InquireVariable failed for var" + std::to_string(tid);
                            return;
                        }

                        auto ctx = reader.NewGetContext();

                        const auto sel = adios2::Selection::All().WithSteps(0, 1);
                        std::vector<double> buf(Nx, -1.0);
                        reader.Get(*ctx, var, buf.data(), sel);
                        reader.PerformGets(*ctx);

                        for (size_t i = 0; i < Nx; ++i)
                        {
                            double expected = static_cast<double>(0 * 1000 + tid * 100 + i);
                            if (buf[i] != expected)
                            {
                                errors[tid] = "var" + std::to_string(tid) + "[" +
                                              std::to_string(i) + "] = " + std::to_string(buf[i]) +
                                              ", expected " + std::to_string(expected);
                                return;
                            }
                        }

                        const auto sel2 = adios2::Selection::All().WithSteps(2, 1);
                        std::fill(buf.begin(), buf.end(), -1.0);
                        reader.Get(*ctx, var, buf.data(), sel2);
                        reader.PerformGets(*ctx);

                        for (size_t i = 0; i < Nx; ++i)
                        {
                            double expected = static_cast<double>(2 * 1000 + tid * 100 + i);
                            if (buf[i] != expected)
                            {
                                errors[tid] = "var" + std::to_string(tid) + " step2[" +
                                              std::to_string(i) + "] = " + std::to_string(buf[i]) +
                                              ", expected " + std::to_string(expected);
                                return;
                            }
                        }
                    }
                    catch (const std::exception &e)
                    {
                        errors[tid] = std::string("exception: ") + e.what();
                    }
                },
                t);
        }

        for (auto &th : threads)
            th.join();

        for (size_t t = 0; t < NumThreads; ++t)
        {
            EXPECT_TRUE(errors[t].empty()) << "Thread " << t << ": " << errors[t];
        }

        reader.Close();
    }

#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
}

// Non-reentrant transport (fstream) — NewGetContext must return null.
TEST_F(BPGetContextIsolation, NonReentrantTransportRejected)
{
    const size_t Nx = 8;
    const std::string fname("BPGetContextNonReentrant.bp");

    adios2::ADIOS adios;
    {
        adios2::IO io = adios.DeclareIO("WriteIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        auto v0 = io.DefineVariable<std::int32_t>("v0", {Nx}, {0}, {Nx});
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        std::vector<std::int32_t> data(Nx, 42);
        writer.BeginStep();
        writer.Put(v0, data.data());
        writer.EndStep();
        writer.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO_fstream");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        io.AddTransport("File", {{"Library", "fstream"}});
        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);

        auto ctx = reader.NewGetContext();
        EXPECT_EQ(ctx, nullptr) << "fstream transport is not reentrant; "
                                   "NewGetContext must return nullptr";

        reader.Close();
    }

#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
}

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    int provided;
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
#endif

    ::testing::InitGoogleTest(&argc, argv);
    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    if (argc > 2)
    {
        engineParameters = std::string(argv[2]);
    }
    int result = RUN_ALL_TESTS();

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
}
