/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <numeric> //std::iota
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

std::string engineName; // comes from command line

void SZ3Accuracy1D(const std::string accuracy)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const size_t Nx = 1000;

    // Number of steps
    const size_t NSteps = 1;

    std::vector<float> r32s(Nx);
    std::vector<double> r64s(Nx);
    std::vector<std::complex<float>> c32s(Nx);
    std::vector<std::complex<double>> c64s(Nx);

    // range 0 to 999
    std::iota(r32s.begin(), r32s.end(), 0.f);
    std::iota(r64s.begin(), r64s.end(), 0.);

    // Initialize complex arrays with both real and imaginary parts
    for (size_t i = 0; i < Nx; ++i)
    {
        c32s[i] = std::complex<float>(static_cast<float>(i), static_cast<float>(i) + 0.5f);
        c64s[i] = std::complex<double>(static_cast<double>(i), static_cast<double>(i) + 0.5);
    }

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    const std::string fname("BPWRSZ3_1D_" + accuracy + "_MPI.bp");
#else
    const std::string fname("BPWRSZ3_1D_" + accuracy + ".bp");
#endif

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize)};
        const adios2::Dims start{static_cast<size_t>(Nx * mpiRank)};
        const adios2::Dims count{Nx};

        adios2::Variable<float> var_r32 =
            io.DefineVariable<float>("r32", shape, start, count, adios2::ConstantDims);
        adios2::Variable<double> var_r64 =
            io.DefineVariable<double>("r64", shape, start, count, adios2::ConstantDims);
        adios2::Variable<std::complex<float>> var_c32 =
            io.DefineVariable<std::complex<float>>("c32", shape, start, count,
                                                    adios2::ConstantDims);
        adios2::Variable<std::complex<double>> var_c64 =
            io.DefineVariable<std::complex<double>>("c64", shape, start, count,
                                                     adios2::ConstantDims);

        // add operations
        adios2::Operator sz3Op = adios.DefineOperator("sz3Compressor", "sz3");

        var_r32.AddOperation(sz3Op, {{"accuracy", accuracy}});
        var_r64.AddOperation(sz3Op, {{"accuracy", accuracy}});
        var_c32.AddOperation(sz3Op, {{"accuracy", accuracy}});
        var_c64.AddOperation(sz3Op, {{"accuracy", accuracy}});

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            bpWriter.BeginStep();
            bpWriter.Put<float>("r32", r32s.data());
            bpWriter.Put<double>("r64", r64s.data());
            bpWriter.Put<std::complex<float>>("c32", c32s.data());
            bpWriter.Put<std::complex<double>>("c64", c64s.data());
            bpWriter.EndStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        unsigned int t = 0;
        std::vector<float> decompressedR32s;
        std::vector<double> decompressedR64s;
        std::vector<std::complex<float>> decompressedC32s;
        std::vector<std::complex<double>> decompressedC64s;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            auto var_r32 = io.InquireVariable<float>("r32");
            EXPECT_TRUE(var_r32);
            ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r32.Steps(), NSteps);
            ASSERT_EQ(var_r32.Shape()[0], mpiSize * Nx);

            auto var_r64 = io.InquireVariable<double>("r64");
            EXPECT_TRUE(var_r64);
            ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r64.Steps(), NSteps);
            ASSERT_EQ(var_r64.Shape()[0], mpiSize * Nx);

            auto var_c32 = io.InquireVariable<std::complex<float>>("c32");
            EXPECT_TRUE(var_c32);
            ASSERT_EQ(var_c32.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_c32.Steps(), NSteps);
            ASSERT_EQ(var_c32.Shape()[0], mpiSize * Nx);

            auto var_c64 = io.InquireVariable<std::complex<double>>("c64");
            EXPECT_TRUE(var_c64);
            ASSERT_EQ(var_c64.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_c64.Steps(), NSteps);
            ASSERT_EQ(var_c64.Shape()[0], mpiSize * Nx);

            const adios2::Dims start{mpiRank * Nx};
            const adios2::Dims count{Nx};
            const adios2::Box<adios2::Dims> sel(start, count);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);
            var_c32.SetSelection(sel);
            var_c64.SetSelection(sel);

            bpReader.Get(var_r32, decompressedR32s);
            bpReader.Get(var_r64, decompressedR64s);
            bpReader.Get(var_c32, decompressedC32s);
            bpReader.Get(var_c64, decompressedC64s);
            bpReader.EndStep();

            for (size_t i = 0; i < Nx; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                ASSERT_LT(std::abs(decompressedR32s[i] - r32s[i]), std::stod(accuracy)) << msg;
                ASSERT_LT(std::abs(decompressedR64s[i] - r64s[i]), std::stod(accuracy)) << msg;
                // For complex, check both real and imaginary parts
                ASSERT_LT(std::abs(decompressedC32s[i].real() - c32s[i].real()),
                          std::stod(accuracy))
                    << msg;
                ASSERT_LT(std::abs(decompressedC32s[i].imag() - c32s[i].imag()),
                          std::stod(accuracy))
                    << msg;
                ASSERT_LT(std::abs(decompressedC64s[i].real() - c64s[i].real()),
                          std::stod(accuracy))
                    << msg;
                ASSERT_LT(std::abs(decompressedC64s[i].imag() - c64s[i].imag()),
                          std::stod(accuracy))
                    << msg;
            }
            ++t;
        }

        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }
}

void SZ3Accuracy2D(const std::string accuracy)
{
    int mpiRank = 0, mpiSize = 1;
    const size_t Nx = 100;
    const size_t Ny = 50;
    const size_t NSteps = 1;

    std::vector<float> r32s(Nx * Ny);
    std::vector<double> r64s(Nx * Ny);
    std::vector<std::complex<float>> c32s(Nx * Ny);
    std::vector<std::complex<double>> c64s(Nx * Ny);

    std::iota(r32s.begin(), r32s.end(), 0.f);
    std::iota(r64s.begin(), r64s.end(), 0.);

    // Initialize complex arrays with both real and imaginary parts
    for (size_t i = 0; i < Nx * Ny; ++i)
    {
        c32s[i] = std::complex<float>(static_cast<float>(i), static_cast<float>(i) + 0.5f);
        c64s[i] = std::complex<double>(static_cast<double>(i), static_cast<double>(i) + 0.5);
    }

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    const std::string fname("BPWRSZ3_2D_" + accuracy + "_MPI.bp");
#else
    const std::string fname("BPWRSZ3_2D_" + accuracy + ".bp");
#endif

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            io.SetEngine("BPFile");
        }

        const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize), Ny};
        const adios2::Dims start{static_cast<size_t>(Nx * mpiRank), 0};
        const adios2::Dims count{Nx, Ny};

        auto var_r32 = io.DefineVariable<float>("r32", shape, start, count, adios2::ConstantDims);
        auto var_r64 = io.DefineVariable<double>("r64", shape, start, count, adios2::ConstantDims);
        auto var_c32 =
            io.DefineVariable<std::complex<float>>("c32", shape, start, count, adios2::ConstantDims);
        auto var_c64 = io.DefineVariable<std::complex<double>>("c64", shape, start, count,
                                                                 adios2::ConstantDims);

        adios2::Operator sz3Op = adios.DefineOperator("sz3Compressor", "sz3");

        var_r32.AddOperation(sz3Op, {{"accuracy", accuracy}});
        var_r64.AddOperation(sz3Op, {{"accuracy", accuracy}});
        var_c32.AddOperation(sz3Op, {{"accuracy", accuracy}});
        var_c64.AddOperation(sz3Op, {{"accuracy", accuracy}});

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            bpWriter.BeginStep();
            bpWriter.Put<float>("r32", r32s.data());
            bpWriter.Put<double>("r64", r64s.data());
            bpWriter.Put<std::complex<float>>("c32", c32s.data());
            bpWriter.Put<std::complex<double>>("c64", c64s.data());
            bpWriter.EndStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            io.SetEngine("BPFile");
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        unsigned int t = 0;
        std::vector<float> decompressedR32s;
        std::vector<double> decompressedR64s;
        std::vector<std::complex<float>> decompressedC32s;
        std::vector<std::complex<double>> decompressedC64s;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            auto var_r32 = io.InquireVariable<float>("r32");
            EXPECT_TRUE(var_r32);
            ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r32.Steps(), NSteps);
            ASSERT_EQ(var_r32.Shape()[0], mpiSize * Nx);
            ASSERT_EQ(var_r32.Shape()[1], Ny);

            auto var_r64 = io.InquireVariable<double>("r64");
            EXPECT_TRUE(var_r64);
            ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r64.Steps(), NSteps);
            ASSERT_EQ(var_r64.Shape()[0], mpiSize * Nx);
            ASSERT_EQ(var_r64.Shape()[1], Ny);

            auto var_c32 = io.InquireVariable<std::complex<float>>("c32");
            EXPECT_TRUE(var_c32);
            ASSERT_EQ(var_c32.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_c32.Steps(), NSteps);
            ASSERT_EQ(var_c32.Shape()[0], mpiSize * Nx);
            ASSERT_EQ(var_c32.Shape()[1], Ny);

            auto var_c64 = io.InquireVariable<std::complex<double>>("c64");
            EXPECT_TRUE(var_c64);
            ASSERT_EQ(var_c64.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_c64.Steps(), NSteps);
            ASSERT_EQ(var_c64.Shape()[0], mpiSize * Nx);
            ASSERT_EQ(var_c64.Shape()[1], Ny);

            const adios2::Dims start{mpiRank * Nx, 0};
            const adios2::Dims count{Nx, Ny};
            const adios2::Box<adios2::Dims> sel(start, count);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);
            var_c32.SetSelection(sel);
            var_c64.SetSelection(sel);

            bpReader.Get(var_r32, decompressedR32s);
            bpReader.Get(var_r64, decompressedR64s);
            bpReader.Get(var_c32, decompressedC32s);
            bpReader.Get(var_c64, decompressedC64s);
            bpReader.EndStep();

            for (size_t i = 0; i < Nx * Ny; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                ASSERT_LT(std::abs(decompressedR32s[i] - r32s[i]), std::stod(accuracy)) << msg;
                ASSERT_LT(std::abs(decompressedR64s[i] - r64s[i]), std::stod(accuracy)) << msg;
                // For complex, check both real and imaginary parts
                ASSERT_LT(std::abs(decompressedC32s[i].real() - c32s[i].real()),
                          std::stod(accuracy))
                    << msg;
                ASSERT_LT(std::abs(decompressedC32s[i].imag() - c32s[i].imag()),
                          std::stod(accuracy))
                    << msg;
                ASSERT_LT(std::abs(decompressedC64s[i].real() - c64s[i].real()),
                          std::stod(accuracy))
                    << msg;
                ASSERT_LT(std::abs(decompressedC64s[i].imag() - c64s[i].imag()),
                          std::stod(accuracy))
                    << msg;
            }
            ++t;
        }

        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }
}

class BPWriteReadSZ3 : public ::testing::TestWithParam<std::string>
{
public:
    BPWriteReadSZ3() = default;
    virtual void SetUp(){};
    virtual void TearDown(){};
};

TEST_P(BPWriteReadSZ3, BPWRSZ3_1D) { SZ3Accuracy1D(GetParam()); }
TEST_P(BPWriteReadSZ3, BPWRSZ3_2D) { SZ3Accuracy2D(GetParam()); }

INSTANTIATE_TEST_SUITE_P(SZ3Accuracy, BPWriteReadSZ3, ::testing::Values("0.01", "0.001", "0.0001"));

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    int provided;
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    result = RUN_ALL_TESTS();

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
}
