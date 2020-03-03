/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include <memory>
#include <stdexcept>
#include <string>

#include <adios2.h>

#include <gtest/gtest.h>

std::string engineName; // comes from command line

class Write : public ::testing::Test
{
public:
    Write() = default;
};

TEST_F(Write, Plain1D_1B)
{
    const std::string fname("LargeVar_Write_Plain1D_1B.bp");

    int mpiRank = 0, mpiSize = 1;

    const std::size_t Nx = 1096 * 1024 * 1024;

    std::unique_ptr<float[]> dataBig(new float[Nx]);

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    // Write test data using ADIOS2

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    adios2::IO io = adios.DeclareIO("TestIO");
    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }

    adios2::Dims shape{Nx};
    adios2::Dims start{(Nx / mpiSize) * mpiRank};
    adios2::Dims count{Nx / mpiSize};

    auto varBig = io.DefineVariable<float>("varBig", shape, start, count,
                                           adios2::ConstantDims);

    adios2::Engine writer = io.Open(fname, adios2::Mode::Write);

    writer.BeginStep();
    writer.Put(varBig, dataBig.get());
    writer.EndStep();
    writer.Close();
}

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
