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

#include "../SmallTestData.h"

class BPWriteReadBlockInfo : public ::testing::Test
{
public:
    BPWriteReadBlockInfo() = default;

    SmallTestData m_TestData;
};

namespace
{

template <class T>
void CheckAllStepsBlockInfo1D(
    const std::vector<std::vector<typename adios2::Variable<T>::Info>>
        &allStepsBlocksInfo,
    const size_t NSteps, const size_t Nx)
{
    EXPECT_EQ(allStepsBlocksInfo.size(), NSteps);

    for (size_t s = 0; s < allStepsBlocksInfo.size(); ++s)
    {
        for (size_t b = 0; b < allStepsBlocksInfo[s].size(); ++b)
        {
            EXPECT_EQ(allStepsBlocksInfo[s][b].BlockID, b);
            EXPECT_EQ(allStepsBlocksInfo[s][b].Start[0], b * Nx);
            EXPECT_EQ(allStepsBlocksInfo[s][b].Count[0], Nx);
            EXPECT_EQ(allStepsBlocksInfo[s][b].Step, s);
        }
    }
}

template <class T>
void CheckAllStepsBlockInfo2D(
    const std::vector<std::vector<typename adios2::Variable<T>::Info>>
        &allStepsBlocksInfo,
    const size_t NSteps, const size_t Nx, const size_t Ny)
{
    EXPECT_EQ(allStepsBlocksInfo.size(), NSteps);

    for (size_t s = 0; s < allStepsBlocksInfo.size(); ++s)
    {
        for (size_t b = 0; b < allStepsBlocksInfo[s].size(); ++b)
        {
            EXPECT_EQ(allStepsBlocksInfo[s][b].BlockID, b);
            EXPECT_EQ(allStepsBlocksInfo[s][b].Start[0], 0);
            EXPECT_EQ(allStepsBlocksInfo[s][b].Start[1], b * Nx);
            EXPECT_EQ(allStepsBlocksInfo[s][b].Count[0], Ny);
            EXPECT_EQ(allStepsBlocksInfo[s][b].Count[1], Nx);
            EXPECT_EQ(allStepsBlocksInfo[s][b].Step, s);
        }
    }
}
}

TEST_F(BPWriteReadBlockInfo, BPWriteReadBlockInfo1D8)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    const std::string fname("BPWriteReadblockInfo1D8.bp");

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const size_t Nx = 8;

    // Number of steps
    const size_t NSteps = 3;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    // Write test data using BP

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize)};
        const adios2::Dims start{static_cast<size_t>(Nx * mpiRank)};
        const adios2::Dims count{Nx};

        auto var_local =
            io.DefineVariable<int32_t>("local", {adios2::LocalValueDim});
        auto var_localStr =
            io.DefineVariable<std::string>("localStr", {adios2::LocalValueDim});

        auto var_iString = io.DefineVariable<std::string>("iString");
        auto var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
        auto var_i16 = io.DefineVariable<int16_t>("i16", shape, start, count);
        auto var_i32 = io.DefineVariable<int32_t>("i32", shape, start, count);
        auto var_i64 = io.DefineVariable<int64_t>("i64", shape, start, count);
        auto var_u8 = io.DefineVariable<uint8_t>("u8", shape, start, count);
        auto var_u16 = io.DefineVariable<uint16_t>("u16", shape, start, count);
        auto var_u32 = io.DefineVariable<uint32_t>("u32", shape, start, count);
        auto var_u64 = io.DefineVariable<uint64_t>("u64", shape, start, count);
        auto var_r32 = io.DefineVariable<float>("r32", shape, start, count);
        auto var_r64 = io.DefineVariable<double>("r64", shape, start, count);
        auto var_cr32 =
            io.DefineVariable<std::complex<float>>("cr32", shape, start, count);
        auto var_cr64 = io.DefineVariable<std::complex<double>>("cr64", shape,
                                                                start, count);

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(step), mpiRank, mpiSize);

            bpWriter.BeginStep();

            const int32_t localNumber = static_cast<int32_t>(mpiRank + step);
            bpWriter.Put(var_local, localNumber);
            bpWriter.Put(var_localStr, std::to_string(localNumber));

            bpWriter.Put(var_iString, currentTestData.S1);
            bpWriter.Put(var_i8, currentTestData.I8.data());
            bpWriter.Put(var_i16, currentTestData.I16.data());
            bpWriter.Put(var_i32, currentTestData.I32.data());
            bpWriter.Put(var_i64, currentTestData.I64.data());
            bpWriter.Put(var_u8, currentTestData.U8.data());
            bpWriter.Put(var_u16, currentTestData.U16.data());
            bpWriter.Put(var_u32, currentTestData.U32.data());
            bpWriter.Put(var_u64, currentTestData.U64.data());
            bpWriter.Put(var_r32, currentTestData.R32.data());
            bpWriter.Put(var_r64, currentTestData.R64.data());
            bpWriter.Put(var_cr32, currentTestData.CR32.data());
            bpWriter.Put(var_cr64, currentTestData.CR64.data());
            bpWriter.EndStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        auto var_local = io.InquireVariable<int32_t>("local");
        auto var_localStr = io.InquireVariable<std::string>("localStr");

        auto var_iString = io.InquireVariable<std::string>("iString");
        auto var_i8 = io.InquireVariable<int8_t>("i8");
        auto var_i16 = io.InquireVariable<int16_t>("i16");
        auto var_i32 = io.InquireVariable<int32_t>("i32");
        auto var_i64 = io.InquireVariable<int64_t>("i64");
        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        auto var_r32 = io.InquireVariable<float>("r32");
        auto var_r64 = io.InquireVariable<double>("r64");
        auto var_cr32 = io.InquireVariable<std::complex<float>>("cr32");
        auto var_cr64 = io.InquireVariable<std::complex<double>>("cr64");

        const std::vector<std::vector<adios2::Variable<int8_t>::Info>>
            allStepsBlocksInfoI8 = var_i8.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<int16_t>::Info>>
            allStepsBlocksInfoI16 = var_i16.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<int32_t>::Info>>
            allStepsBlocksInfoI32 = var_i32.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<int64_t>::Info>>
            allStepsBlocksInfoI64 = var_i64.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<uint8_t>::Info>>
            allStepsBlocksInfoU8 = var_u8.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<uint16_t>::Info>>
            allStepsBlocksInfoU16 = var_u16.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<uint32_t>::Info>>
            allStepsBlocksInfoU32 = var_u32.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<uint64_t>::Info>>
            allStepsBlocksInfoU64 = var_u64.AllStepsBlocksInfo();

        const std::vector<std::vector<adios2::Variable<float>::Info>>
            allStepsBlocksInfoR32 = var_r32.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<double>::Info>>
            allStepsBlocksInfoR64 = var_r64.AllStepsBlocksInfo();

        const std::vector<
            std::vector<adios2::Variable<std::complex<float>>::Info>>
            allStepsBlocksInfoCR32 = var_cr32.AllStepsBlocksInfo();
        const std::vector<
            std::vector<adios2::Variable<std::complex<double>>::Info>>
            allStepsBlocksInfoCR64 = var_cr64.AllStepsBlocksInfo();

        EXPECT_EQ(allStepsBlocksInfoI8.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoI16.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoI32.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoI64.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoU8.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoU16.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoU32.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoU64.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoR32.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoR64.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoCR32.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoCR64.size(), NSteps);

        CheckAllStepsBlockInfo1D<int8_t>(allStepsBlocksInfoI8, NSteps, Nx);
        CheckAllStepsBlockInfo1D<int16_t>(allStepsBlocksInfoI16, NSteps, Nx);
        CheckAllStepsBlockInfo1D<int32_t>(allStepsBlocksInfoI32, NSteps, Nx);
        CheckAllStepsBlockInfo1D<int64_t>(allStepsBlocksInfoI64, NSteps, Nx);
        CheckAllStepsBlockInfo1D<uint8_t>(allStepsBlocksInfoU8, NSteps, Nx);
        CheckAllStepsBlockInfo1D<uint16_t>(allStepsBlocksInfoU16, NSteps, Nx);
        CheckAllStepsBlockInfo1D<uint32_t>(allStepsBlocksInfoU32, NSteps, Nx);
        CheckAllStepsBlockInfo1D<uint64_t>(allStepsBlocksInfoU64, NSteps, Nx);
        CheckAllStepsBlockInfo1D<float>(allStepsBlocksInfoR32, NSteps, Nx);
        CheckAllStepsBlockInfo1D<double>(allStepsBlocksInfoR64, NSteps, Nx);
        CheckAllStepsBlockInfo1D<std::complex<float>>(allStepsBlocksInfoCR32,
                                                      NSteps, Nx);
        CheckAllStepsBlockInfo1D<std::complex<double>>(allStepsBlocksInfoCR64,
                                                       NSteps, Nx);

        // TODO: other types

        SmallTestData testData;
        std::vector<int32_t> ILocal;
        std::vector<std::string> ILocalStr;

        std::string IString;
        std::array<int8_t, Nx> I8;
        std::array<int16_t, Nx> I16;
        std::array<int32_t, Nx> I32;
        std::array<int64_t, Nx> I64;
        std::array<uint8_t, Nx> U8;
        std::array<uint16_t, Nx> U16;
        std::array<uint32_t, Nx> U32;
        std::array<uint64_t, Nx> U64;
        std::array<float, Nx> R32;
        std::array<double, Nx> R64;
        std::array<std::complex<float>, Nx> CR32;
        std::array<std::complex<double>, Nx> CR64;

        const size_t blockRank = static_cast<size_t>(mpiRank);
        var_i8.SetBlockSelection(blockRank);
        var_i16.SetBlockSelection(blockRank);
        var_i32.SetBlockSelection(blockRank);
        var_i64.SetBlockSelection(blockRank);

        var_u8.SetBlockSelection(blockRank);
        var_u16.SetBlockSelection(blockRank);
        var_u32.SetBlockSelection(blockRank);
        var_u64.SetBlockSelection(blockRank);

        var_r32.SetBlockSelection(blockRank);
        var_r64.SetBlockSelection(blockRank);

        var_cr32.SetBlockSelection(blockRank);
        var_cr64.SetBlockSelection(blockRank);

        for (size_t t = 0; t < NSteps; ++t)
        {
            var_local.SetStepSelection({t, 1});
            var_localStr.SetStepSelection({t, 1});

            var_i8.SetStepSelection({t, 1});
            var_i16.SetStepSelection({t, 1});
            var_i32.SetStepSelection({t, 1});
            var_i64.SetStepSelection({t, 1});

            var_u8.SetStepSelection({t, 1});
            var_u16.SetStepSelection({t, 1});
            var_u32.SetStepSelection({t, 1});
            var_u64.SetStepSelection({t, 1});

            var_r32.SetStepSelection({t, 1});
            var_r64.SetStepSelection({t, 1});
            var_cr32.SetStepSelection({t, 1});
            var_cr64.SetStepSelection({t, 1});

            // Generate test data for each rank uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(t), mpiRank, mpiSize);

            bpReader.Get(var_iString, IString);

            bpReader.Get(var_i8, I8.data());
            bpReader.Get(var_i16, I16.data());
            bpReader.Get(var_i32, I32.data());
            bpReader.Get(var_i64, I64.data());

            bpReader.Get(var_u8, U8.data());
            bpReader.Get(var_u16, U16.data());
            bpReader.Get(var_u32, U32.data());
            bpReader.Get(var_u64, U64.data());

            bpReader.Get(var_r32, R32.data());
            bpReader.Get(var_r64, R64.data());
            bpReader.Get(var_cr32, CR32.data());
            bpReader.Get(var_cr64, CR64.data());

            bpReader.PerformGets();

            EXPECT_EQ(IString, currentTestData.S1);

            for (size_t i = 0; i < Nx; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], currentTestData.I8[i]) << msg;
                EXPECT_EQ(I16[i], currentTestData.I16[i]) << msg;
                EXPECT_EQ(I32[i], currentTestData.I32[i]) << msg;
                EXPECT_EQ(I64[i], currentTestData.I64[i]) << msg;
                EXPECT_EQ(U8[i], currentTestData.U8[i]) << msg;
                EXPECT_EQ(U16[i], currentTestData.U16[i]) << msg;
                EXPECT_EQ(U32[i], currentTestData.U32[i]) << msg;
                EXPECT_EQ(U64[i], currentTestData.U64[i]) << msg;
                EXPECT_EQ(R32[i], currentTestData.R32[i]) << msg;
                EXPECT_EQ(R64[i], currentTestData.R64[i]) << msg;
                EXPECT_EQ(CR32[i], currentTestData.CR32[i]) << msg;
                EXPECT_EQ(CR64[i], currentTestData.CR64[i]) << msg;
            }

            const size_t domainSize = static_cast<size_t>(mpiSize);
            for (size_t i = 0; i < domainSize; ++i)
            {
                var_local.SetSelection({{i}, {domainSize - i}});
                var_localStr.SetSelection({{i}, {domainSize - i}});

                bpReader.Get(var_local, ILocal);
                bpReader.Get(var_localStr, ILocalStr);
                bpReader.PerformGets();

                for (size_t j = i; j < domainSize; ++j)
                {
                    std::stringstream ss;
                    ss << "t=" << t << " i=" << i << " j=" << j;
                    std::string msg = ss.str();

                    EXPECT_EQ(ILocal[j - i], j + t) << msg;
                    EXPECT_EQ(ILocalStr[j - i], std::to_string(j + t)) << msg;
                }
            }
        }
        bpReader.Close();
    }
}

TEST_F(BPWriteReadBlockInfo, BPWriteReadBlockInfo2D2x4)
{
    const std::string fname("BPWriteReadBlockInfo2D2x4.bp");

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const std::size_t Nx = 4;

    // Number of rows
    const std::size_t Ny = 2;

    // Number of steps
    const std::size_t NSteps = 3;

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
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        const adios2::Dims shape{Ny, static_cast<size_t>(Nx * mpiSize)};
        const adios2::Dims start{0, static_cast<size_t>(mpiRank * Nx)};
        const adios2::Dims count{Ny, Nx};

        auto var_iString = io.DefineVariable<std::string>("iString");
        auto var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
        auto var_i16 = io.DefineVariable<int16_t>("i16", shape, start, count);
        auto var_i32 = io.DefineVariable<int32_t>("i32", shape, start, count);
        auto var_i64 = io.DefineVariable<int64_t>("i64", shape, start, count);
        auto var_u8 = io.DefineVariable<uint8_t>("u8", shape, start, count);
        auto var_u16 = io.DefineVariable<uint16_t>("u16", shape, start, count);
        auto var_u32 = io.DefineVariable<uint32_t>("u32", shape, start, count);
        auto var_u64 = io.DefineVariable<uint64_t>("u64", shape, start, count);
        auto var_r32 = io.DefineVariable<float>("r32", shape, start, count);
        auto var_r64 = io.DefineVariable<double>("r64", shape, start, count);
        auto var_cr32 =
            io.DefineVariable<std::complex<float>>("cr32", shape, start, count);
        auto var_cr64 = io.DefineVariable<std::complex<double>>("cr64", shape,
                                                                start, count);

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(step), mpiRank, mpiSize);

            bpWriter.BeginStep();
            bpWriter.Put(var_iString, currentTestData.S1);
            bpWriter.Put(var_i8, currentTestData.I8.data());
            bpWriter.Put(var_i16, currentTestData.I16.data());
            bpWriter.Put(var_i32, currentTestData.I32.data());
            bpWriter.Put(var_i64, currentTestData.I64.data());
            bpWriter.Put(var_u8, currentTestData.U8.data());
            bpWriter.Put(var_u16, currentTestData.U16.data());
            bpWriter.Put(var_u32, currentTestData.U32.data());
            bpWriter.Put(var_u64, currentTestData.U64.data());
            bpWriter.Put(var_r32, currentTestData.R32.data());
            bpWriter.Put(var_r64, currentTestData.R64.data());
            bpWriter.Put(var_cr32, currentTestData.CR32.data());
            bpWriter.Put(var_cr64, currentTestData.CR64.data());
            bpWriter.EndStep();
        }

        // Close the file
        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        auto var_iString = io.InquireVariable<std::string>("iString");
        auto var_i8 = io.InquireVariable<int8_t>("i8");
        auto var_i16 = io.InquireVariable<int16_t>("i16");
        auto var_i32 = io.InquireVariable<int32_t>("i32");
        auto var_i64 = io.InquireVariable<int64_t>("i64");
        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        auto var_r32 = io.InquireVariable<float>("r32");
        auto var_r64 = io.InquireVariable<double>("r64");
        auto var_cr32 = io.InquireVariable<std::complex<float>>("cr32");
        auto var_cr64 = io.InquireVariable<std::complex<double>>("cr64");

        const std::vector<std::vector<adios2::Variable<int8_t>::Info>>
            allStepsBlocksInfoI8 = var_i8.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<int16_t>::Info>>
            allStepsBlocksInfoI16 = var_i16.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<int32_t>::Info>>
            allStepsBlocksInfoI32 = var_i32.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<int64_t>::Info>>
            allStepsBlocksInfoI64 = var_i64.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<uint8_t>::Info>>
            allStepsBlocksInfoU8 = var_u8.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<uint16_t>::Info>>
            allStepsBlocksInfoU16 = var_u16.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<uint32_t>::Info>>
            allStepsBlocksInfoU32 = var_u32.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<uint64_t>::Info>>
            allStepsBlocksInfoU64 = var_u64.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<float>::Info>>
            allStepsBlocksInfoR32 = var_r32.AllStepsBlocksInfo();
        const std::vector<std::vector<adios2::Variable<double>::Info>>
            allStepsBlocksInfoR64 = var_r64.AllStepsBlocksInfo();
        const std::vector<
            std::vector<adios2::Variable<std::complex<float>>::Info>>
            allStepsBlocksInfoCR32 = var_cr32.AllStepsBlocksInfo();
        const std::vector<
            std::vector<adios2::Variable<std::complex<double>>::Info>>
            allStepsBlocksInfoCR64 = var_cr64.AllStepsBlocksInfo();

        EXPECT_EQ(allStepsBlocksInfoI8.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoI16.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoI32.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoI64.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoU8.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoU16.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoU32.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoU64.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoR32.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoR64.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoCR32.size(), NSteps);
        EXPECT_EQ(allStepsBlocksInfoCR64.size(), NSteps);

        CheckAllStepsBlockInfo2D<int8_t>(allStepsBlocksInfoI8, NSteps, Nx, Ny);
        CheckAllStepsBlockInfo2D<int16_t>(allStepsBlocksInfoI16, NSteps, Nx,
                                          Ny);
        CheckAllStepsBlockInfo2D<int32_t>(allStepsBlocksInfoI32, NSteps, Nx,
                                          Ny);
        CheckAllStepsBlockInfo2D<int64_t>(allStepsBlocksInfoI64, NSteps, Nx,
                                          Ny);
        CheckAllStepsBlockInfo2D<uint8_t>(allStepsBlocksInfoU8, NSteps, Nx, Ny);
        CheckAllStepsBlockInfo2D<uint16_t>(allStepsBlocksInfoU16, NSteps, Nx,
                                           Ny);
        CheckAllStepsBlockInfo2D<uint32_t>(allStepsBlocksInfoU32, NSteps, Nx,
                                           Ny);
        CheckAllStepsBlockInfo2D<uint64_t>(allStepsBlocksInfoU64, NSteps, Nx,
                                           Ny);
        CheckAllStepsBlockInfo2D<float>(allStepsBlocksInfoR32, NSteps, Nx, Ny);
        CheckAllStepsBlockInfo2D<double>(allStepsBlocksInfoR64, NSteps, Nx, Ny);
        CheckAllStepsBlockInfo2D<std::complex<float>>(allStepsBlocksInfoCR32,
                                                      NSteps, Nx, Ny);
        CheckAllStepsBlockInfo2D<std::complex<double>>(allStepsBlocksInfoCR64,
                                                       NSteps, Nx, Ny);

        std::string IString;
        std::array<int8_t, Nx * Ny> I8;
        std::array<int16_t, Nx * Ny> I16;
        std::array<int32_t, Nx * Ny> I32;
        std::array<int64_t, Nx * Ny> I64;
        std::array<uint8_t, Nx * Ny> U8;
        std::array<uint16_t, Nx * Ny> U16;
        std::array<uint32_t, Nx * Ny> U32;
        std::array<uint64_t, Nx * Ny> U64;
        std::array<float, Nx * Ny> R32;
        std::array<double, Nx * Ny> R64;
        std::array<std::complex<float>, Nx * Ny> CR32;
        std::array<std::complex<double>, Nx * Ny> CR64;

        const size_t blockRank = static_cast<size_t>(mpiRank);
        var_i8.SetBlockSelection(blockRank);
        var_i16.SetBlockSelection(blockRank);
        var_i32.SetBlockSelection(blockRank);
        var_i64.SetBlockSelection(blockRank);

        var_u8.SetBlockSelection(blockRank);
        var_u16.SetBlockSelection(blockRank);
        var_u32.SetBlockSelection(blockRank);
        var_u64.SetBlockSelection(blockRank);

        var_r32.SetBlockSelection(blockRank);
        var_r64.SetBlockSelection(blockRank);
        var_cr32.SetBlockSelection(blockRank);
        var_cr64.SetBlockSelection(blockRank);

        for (size_t t = 0; t < NSteps; ++t)
        {
            var_i8.SetStepSelection({t, 1});
            var_i16.SetStepSelection({t, 1});
            var_i32.SetStepSelection({t, 1});
            var_i64.SetStepSelection({t, 1});

            var_u8.SetStepSelection({t, 1});
            var_u16.SetStepSelection({t, 1});
            var_u32.SetStepSelection({t, 1});
            var_u64.SetStepSelection({t, 1});

            var_r32.SetStepSelection({t, 1});
            var_r64.SetStepSelection({t, 1});
            var_cr32.SetStepSelection({t, 1});
            var_cr64.SetStepSelection({t, 1});

            bpReader.Get(var_iString, IString);

            bpReader.Get(var_i8, I8.data());
            bpReader.Get(var_i16, I16.data());
            bpReader.Get(var_i32, I32.data());
            bpReader.Get(var_i64, I64.data());

            bpReader.Get(var_u8, U8.data());
            bpReader.Get(var_u16, U16.data());
            bpReader.Get(var_u32, U32.data());
            bpReader.Get(var_u64, U64.data());

            bpReader.Get(var_r32, R32.data());
            bpReader.Get(var_r64, R64.data());
            bpReader.Get(var_cr32, CR32.data());
            bpReader.Get(var_cr64, CR64.data());

            bpReader.PerformGets();

            // Generate test data for each rank uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(t), mpiRank, mpiSize);

            EXPECT_EQ(IString, currentTestData.S1);

            for (size_t i = 0; i < Nx * Ny; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], currentTestData.I8[i]) << msg;
                EXPECT_EQ(I16[i], currentTestData.I16[i]) << msg;
                EXPECT_EQ(I32[i], currentTestData.I32[i]) << msg;
                EXPECT_EQ(I64[i], currentTestData.I64[i]) << msg;
                EXPECT_EQ(U8[i], currentTestData.U8[i]) << msg;
                EXPECT_EQ(U16[i], currentTestData.U16[i]) << msg;
                EXPECT_EQ(U32[i], currentTestData.U32[i]) << msg;
                EXPECT_EQ(U64[i], currentTestData.U64[i]) << msg;
                EXPECT_EQ(R32[i], currentTestData.R32[i]) << msg;
                EXPECT_EQ(R64[i], currentTestData.R64[i]) << msg;
                EXPECT_EQ(CR32[i], currentTestData.CR32[i]) << msg;
                EXPECT_EQ(CR64[i], currentTestData.CR64[i]) << msg;
            }
        }
        bpReader.Close();
    }
}

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
