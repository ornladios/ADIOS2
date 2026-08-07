/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cmath>
#include <vector>

#include <adios2.h>

#include <gtest/gtest.h>

// Reader-side derived variable over a global array decomposed across ranks. The
// derived var's per-block structure is taken from its (congruent) inputs, so a
// multi-rank read exercises the block-structure redirect that serial cannot.

namespace
{
const std::string expr = "x=vx \n y=vy \n z=vz \n magnitude(x,y,z)";

double value(size_t global_i, int c) { return 1.0 + static_cast<double>(global_i) + 10.0 * c; }

double expectedSpeed(size_t global_i)
{
    const double x = value(global_i, 0);
    const double y = value(global_i, 1);
    const double z = value(global_i, 2);
    return std::sqrt(x * x + y * y + z * z);
}
} // namespace

TEST(ReaderDerivedMPI, DecomposedGlobalArray)
{
    int mpiRank = 0, mpiSize = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

    const size_t Nlocal = 10;
    const size_t Nglobal = Nlocal * static_cast<size_t>(mpiSize);
    const size_t start = Nlocal * static_cast<size_t>(mpiRank);
    const std::string fname = "ReaderDerivedMPI.bp";

    adios2::ADIOS adios(MPI_COMM_WORLD);
    {
        adios2::IO io = adios.DeclareIO("w");
        io.SetEngine("BP5");
        auto vx = io.DefineVariable<double>("vx", {Nglobal}, {start}, {Nlocal});
        auto vy = io.DefineVariable<double>("vy", {Nglobal}, {start}, {Nlocal});
        auto vz = io.DefineVariable<double>("vz", {Nglobal}, {start}, {Nlocal});
        std::vector<double> dx(Nlocal), dy(Nlocal), dz(Nlocal);
        for (size_t i = 0; i < Nlocal; i++)
        {
            dx[i] = value(start + i, 0);
            dy[i] = value(start + i, 1);
            dz[i] = value(start + i, 2);
        }
        auto w = io.Open(fname, adios2::Mode::Write);
        w.BeginStep();
        w.Put(vx, dx.data());
        w.Put(vy, dy.data());
        w.Put(vz, dz.data());
        w.EndStep();
        w.Close();
    }

    adios2::IO io = adios.DeclareIO("r");
    io.SetEngine("BP5");
    io.DefineReaderDerivedVariable("speed", expr);
    auto r = io.Open(fname, adios2::Mode::Read);

    ASSERT_EQ(r.BeginStep(), adios2::StepStatus::OK);
    auto speed = io.InquireVariable<double>("speed");
    ASSERT_TRUE(speed);
    EXPECT_EQ(speed.Shape()[0], Nglobal);

    // Read this rank's own block back and check it against the global indices.
    speed.SetSelection({{start}, {Nlocal}});
    std::vector<double> out;
    r.Get(speed, out);
    r.EndStep();
    r.Close();

    ASSERT_EQ(out.size(), Nlocal);
    for (size_t i = 0; i < Nlocal; i++)
        EXPECT_NEAR(out[i], expectedSpeed(start + i), 1e-9);
}

int main(int argc, char **argv)
{
    int provided;
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    MPI_Finalize();
    return result;
}
