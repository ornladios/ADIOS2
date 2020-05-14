/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <algorithm> //std::min_element, std::max_element
#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "SmallTestData250.h"

std::string dataPath;   // comes from command line
std::string engineName; // comes from command line

class ReadMultiblock : public ::testing::Test
{
public:
    ReadMultiblock() = default;

    SmallTestData250 m_TestData;
};

//******************************************************************************
// 2D 4x2 test data
//******************************************************************************

TEST_F(ReadMultiblock, 2D4x2)
{
    /* Data: Written by 4 processes
         Each process did write a 4x2 array in two blocks of 2x2
           Writer process pattern in array
           (rank 0 writes two blocks non-contiguously in global array)
           0 1 2 3
           0 1 2 3
         Blocks position in array (rank 0 wrote block 0 and 1, etc)
           0 2 4 6
           1 3 5 7
    */
    const std::string fname(dataPath + std::string(&adios2::PathSeparator, 1) +
                            engineName + "Multiblock2D4x2.bp");
    // std::cout << "Open file: " << fname << std::endl;
    // Number of columns per block
    const std::size_t Nx = 2;
    // Number of rows per block
    const std::size_t Ny = 2;

    // Number of blocks comprising each array in the file
    const std::size_t NBlocksX = 4;
    const std::size_t NBlocksY = 2;
    const std::size_t NBlocksTotal = NBlocksY * NBlocksX;

    // Number of total columns of each array in file
    const std::size_t Gnx = NBlocksX * Nx;
    // Number of total rows of each array in file
    const std::size_t Gny = NBlocksY * Ny;

    // Number of steps
    const std::size_t NSteps = 3;

    adios2::ADIOS adios;

    adios2::IO io = adios.DeclareIO("ReadIO");

    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }

    adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

    auto var_i8 = io.InquireVariable<int8_t>("i8");
    EXPECT_TRUE(var_i8);
    EXPECT_EQ(var_i8.ShapeID(), adios2::ShapeID::GlobalArray);
    EXPECT_EQ(var_i8.Steps(), NSteps);
    EXPECT_EQ(var_i8.Shape()[0], Gny);
    EXPECT_EQ(var_i8.Shape()[1], Gnx);

    auto var_i16 = io.InquireVariable<int16_t>("i16");
    EXPECT_TRUE(var_i16);
    EXPECT_EQ(var_i16.ShapeID(), adios2::ShapeID::GlobalArray);
    EXPECT_EQ(var_i16.Steps(), NSteps);
    EXPECT_EQ(var_i16.Shape()[0], Gny);
    EXPECT_EQ(var_i16.Shape()[1], Gnx);

    auto var_i32 = io.InquireVariable<int32_t>("i32");
    EXPECT_TRUE(var_i32);
    EXPECT_EQ(var_i32.ShapeID(), adios2::ShapeID::GlobalArray);
    EXPECT_EQ(var_i32.Steps(), NSteps);
    EXPECT_EQ(var_i32.Shape()[0], Gny);
    EXPECT_EQ(var_i32.Shape()[1], Gnx);

    auto var_i64 = io.InquireVariable<int64_t>("i64");
    EXPECT_TRUE(var_i64);
    EXPECT_EQ(var_i64.ShapeID(), adios2::ShapeID::GlobalArray);
    EXPECT_EQ(var_i64.Steps(), NSteps);
    EXPECT_EQ(var_i64.Shape()[0], Gny);
    EXPECT_EQ(var_i64.Shape()[1], Gnx);

    auto var_u8 = io.InquireVariable<uint8_t>("u8");
    EXPECT_TRUE(var_u8);
    EXPECT_EQ(var_u8.ShapeID(), adios2::ShapeID::GlobalArray);
    EXPECT_EQ(var_u8.Steps(), NSteps);
    EXPECT_EQ(var_u8.Shape()[0], Gny);
    EXPECT_EQ(var_u8.Shape()[1], Gnx);

    auto var_u16 = io.InquireVariable<uint16_t>("u16");
    EXPECT_TRUE(var_u16);
    EXPECT_EQ(var_u16.ShapeID(), adios2::ShapeID::GlobalArray);
    EXPECT_EQ(var_u16.Steps(), NSteps);
    EXPECT_EQ(var_u16.Shape()[0], Gny);
    EXPECT_EQ(var_u16.Shape()[1], Gnx);

    auto var_u32 = io.InquireVariable<uint32_t>("u32");
    EXPECT_TRUE(var_u32);
    EXPECT_EQ(var_u32.ShapeID(), adios2::ShapeID::GlobalArray);
    EXPECT_EQ(var_u32.Steps(), NSteps);
    EXPECT_EQ(var_u32.Shape()[0], Gny);
    EXPECT_EQ(var_u32.Shape()[1], Gnx);

    auto var_u64 = io.InquireVariable<uint64_t>("u64");
    EXPECT_TRUE(var_u64);
    EXPECT_EQ(var_u64.ShapeID(), adios2::ShapeID::GlobalArray);
    EXPECT_EQ(var_u64.Steps(), NSteps);
    EXPECT_EQ(var_u64.Shape()[0], Gny);
    EXPECT_EQ(var_u64.Shape()[1], Gnx);

    auto var_r32 = io.InquireVariable<float>("r32");
    EXPECT_TRUE(var_r32);
    EXPECT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
    EXPECT_EQ(var_r32.Steps(), NSteps);
    EXPECT_EQ(var_r32.Shape()[0], Gny);
    EXPECT_EQ(var_r32.Shape()[1], Gnx);

    auto var_r64 = io.InquireVariable<double>("r64");
    EXPECT_TRUE(var_r64);
    EXPECT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
    EXPECT_EQ(var_r64.Steps(), NSteps);
    EXPECT_EQ(var_r64.Shape()[0], Gny);
    EXPECT_EQ(var_r64.Shape()[1], Gnx);

    auto var_cr32 = io.InquireVariable<std::complex<float>>("cr32");
    EXPECT_TRUE(var_cr32);
    EXPECT_EQ(var_cr32.ShapeID(), adios2::ShapeID::GlobalArray);
    EXPECT_EQ(var_cr32.Steps(), NSteps);
    EXPECT_EQ(var_cr32.Shape()[0], Gny);
    EXPECT_EQ(var_cr32.Shape()[1], Gnx);

    auto var_cr64 = io.InquireVariable<std::complex<double>>("cr64");
    EXPECT_TRUE(var_cr64);
    EXPECT_EQ(var_cr64.ShapeID(), adios2::ShapeID::GlobalArray);
    EXPECT_EQ(var_cr64.Steps(), NSteps);
    EXPECT_EQ(var_cr64.Shape()[0], Gny);
    EXPECT_EQ(var_cr64.Shape()[1], Gnx);

    // create arrays to contain one block
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

    size_t t = 0;

    while (bpReader.BeginStep() == adios2::StepStatus::OK)
    {

        std::cout << "Check step t = " << t << std::endl;

        const std::vector<adios2::Variable<int8_t>::Info> i8Info =
            bpReader.BlocksInfo(var_i8, bpReader.CurrentStep());
        const std::vector<adios2::Variable<int16_t>::Info> i16Info =
            bpReader.BlocksInfo(var_i16, bpReader.CurrentStep());
        const std::vector<adios2::Variable<int32_t>::Info> i32Info =
            bpReader.BlocksInfo(var_i32, bpReader.CurrentStep());
        const std::vector<adios2::Variable<int64_t>::Info> i64Info =
            bpReader.BlocksInfo(var_i64, bpReader.CurrentStep());
        const std::vector<adios2::Variable<uint8_t>::Info> u8Info =
            bpReader.BlocksInfo(var_u8, bpReader.CurrentStep());
        const std::vector<adios2::Variable<uint16_t>::Info> u16Info =
            bpReader.BlocksInfo(var_u16, bpReader.CurrentStep());
        const std::vector<adios2::Variable<uint32_t>::Info> u32Info =
            bpReader.BlocksInfo(var_u32, bpReader.CurrentStep());
        const std::vector<adios2::Variable<uint64_t>::Info> u64Info =
            bpReader.BlocksInfo(var_u64, bpReader.CurrentStep());
        const std::vector<adios2::Variable<float>::Info> r32Info =
            bpReader.BlocksInfo(var_r32, bpReader.CurrentStep());
        const std::vector<adios2::Variable<double>::Info> r64Info =
            bpReader.BlocksInfo(var_r64, bpReader.CurrentStep());

        const std::vector<adios2::Variable<std::complex<float>>::Info>
            cr32Info = bpReader.BlocksInfo(var_cr32, bpReader.CurrentStep());
        const std::vector<adios2::Variable<std::complex<double>>::Info>
            cr64Info = bpReader.BlocksInfo(var_cr64, bpReader.CurrentStep());

        EXPECT_EQ(i8Info.size(), NBlocksTotal);
        EXPECT_EQ(i16Info.size(), NBlocksTotal);
        EXPECT_EQ(i32Info.size(), NBlocksTotal);
        EXPECT_EQ(i64Info.size(), NBlocksTotal);
        EXPECT_EQ(u8Info.size(), NBlocksTotal);
        EXPECT_EQ(u16Info.size(), NBlocksTotal);
        EXPECT_EQ(u32Info.size(), NBlocksTotal);
        EXPECT_EQ(u64Info.size(), NBlocksTotal);
        EXPECT_EQ(r32Info.size(), NBlocksTotal);
        EXPECT_EQ(r64Info.size(), NBlocksTotal);
        EXPECT_EQ(cr32Info.size(), NBlocksTotal);
        EXPECT_EQ(cr64Info.size(), NBlocksTotal);

        for (size_t b = 0; b < NBlocksTotal; ++b)
        {
            const size_t inRank = b / 2;
            const size_t blockPosX = inRank;
            // first or second block per process?
            const size_t blockPosY = (b % 2);

            // Make a 2D selection to describe the local dimensions of the
            // block we read and its offsets in the global spaces
            const adios2::Box<adios2::Dims> sel1(
                {blockPosY * Ny, blockPosX * Nx}, {Ny, Nx});

            std::cout << "  Check block b = " << b << " start = {"
                      << sel1.first[0] << ", " << sel1.first[1] << "} count = {"
                      << sel1.second[0] << ", " << sel1.second[1]
                      << "} writer rank = " << inRank << " block pos = ("
                      << blockPosY << "," << blockPosX << ")" << std::endl;

            EXPECT_FALSE(i8Info[b].IsValue);
            EXPECT_FALSE(i16Info[b].IsValue);
            EXPECT_FALSE(i32Info[b].IsValue);
            EXPECT_FALSE(i64Info[b].IsValue);
            EXPECT_FALSE(u8Info[b].IsValue);
            EXPECT_FALSE(u16Info[b].IsValue);
            EXPECT_FALSE(u32Info[b].IsValue);
            EXPECT_FALSE(u64Info[b].IsValue);
            EXPECT_FALSE(r32Info[b].IsValue);
            EXPECT_FALSE(r64Info[b].IsValue);
            EXPECT_FALSE(cr32Info[b].IsValue);
            EXPECT_FALSE(cr64Info[b].IsValue);

            // Generate data for one original writer's two blocks at once
            SmallTestData250 currentTestData =
                generateNewSmallTestData(m_TestData, static_cast<int>(t),
                                         static_cast<int>(inRank), Nx * Ny);

            EXPECT_EQ(i8Info[b].Start[0], blockPosY * Ny);
            EXPECT_EQ(i8Info[b].Start[1], blockPosX * Nx);
            EXPECT_EQ(i8Info[b].Count[0], Ny);
            EXPECT_EQ(i8Info[b].Count[1], Nx);
            EXPECT_EQ(i8Info[b].WriterID, inRank);

            EXPECT_EQ(i16Info[b].Start[0], blockPosY * Ny);
            EXPECT_EQ(i16Info[b].Start[1], blockPosX * Nx);
            EXPECT_EQ(i16Info[b].Count[0], Ny);
            EXPECT_EQ(i16Info[b].Count[1], Nx);
            EXPECT_EQ(i16Info[b].WriterID, inRank);

            EXPECT_EQ(i32Info[b].Start[0], blockPosY * Ny);
            EXPECT_EQ(i32Info[b].Start[1], blockPosX * Nx);
            EXPECT_EQ(i32Info[b].Count[0], Ny);
            EXPECT_EQ(i32Info[b].Count[1], Nx);
            EXPECT_EQ(i32Info[b].WriterID, inRank);

            EXPECT_EQ(i64Info[b].Start[0], blockPosY * Ny);
            EXPECT_EQ(i64Info[b].Start[1], blockPosX * Nx);
            EXPECT_EQ(i64Info[b].Count[0], Ny);
            EXPECT_EQ(i64Info[b].Count[1], Nx);
            EXPECT_EQ(i64Info[b].WriterID, inRank);

            EXPECT_EQ(u8Info[b].Start[0], blockPosY * Ny);
            EXPECT_EQ(u8Info[b].Start[1], blockPosX * Nx);
            EXPECT_EQ(u8Info[b].Count[0], Ny);
            EXPECT_EQ(u8Info[b].Count[1], Nx);
            EXPECT_EQ(u8Info[b].WriterID, inRank);

            EXPECT_EQ(u16Info[b].Start[0], blockPosY * Ny);
            EXPECT_EQ(u16Info[b].Start[1], blockPosX * Nx);
            EXPECT_EQ(u16Info[b].Count[0], Ny);
            EXPECT_EQ(u16Info[b].Count[1], Nx);
            EXPECT_EQ(u16Info[b].WriterID, inRank);

            EXPECT_EQ(u32Info[b].Start[0], blockPosY * Ny);
            EXPECT_EQ(u32Info[b].Start[1], blockPosX * Nx);
            EXPECT_EQ(u32Info[b].Count[0], Ny);
            EXPECT_EQ(u32Info[b].Count[1], Nx);
            EXPECT_EQ(u32Info[b].WriterID, inRank);

            EXPECT_EQ(u64Info[b].Start[0], blockPosY * Ny);
            EXPECT_EQ(u64Info[b].Start[1], blockPosX * Nx);
            EXPECT_EQ(u64Info[b].Count[0], Ny);
            EXPECT_EQ(u64Info[b].Count[1], Nx);
            EXPECT_EQ(u64Info[b].WriterID, inRank);

            EXPECT_EQ(r32Info[b].Start[0], blockPosY * Ny);
            EXPECT_EQ(r32Info[b].Start[1], blockPosX * Nx);
            EXPECT_EQ(r32Info[b].Count[0], Ny);
            EXPECT_EQ(r32Info[b].Count[1], Nx);
            EXPECT_EQ(r32Info[b].WriterID, inRank);

            EXPECT_EQ(r64Info[b].Start[0], blockPosY * Ny);
            EXPECT_EQ(r64Info[b].Start[1], blockPosX * Nx);
            EXPECT_EQ(r64Info[b].Count[0], Ny);
            EXPECT_EQ(r64Info[b].Count[1], Nx);
            EXPECT_EQ(r64Info[b].WriterID, inRank);

            EXPECT_EQ(cr32Info[b].Start[0], blockPosY * Ny);
            EXPECT_EQ(cr32Info[b].Start[1], blockPosX * Nx);
            EXPECT_EQ(cr32Info[b].Count[0], Ny);
            EXPECT_EQ(cr32Info[b].Count[1], Nx);
            EXPECT_EQ(cr32Info[b].WriterID, inRank);

            EXPECT_EQ(cr64Info[b].Start[0], blockPosY * Ny);
            EXPECT_EQ(cr64Info[b].Start[1], inRank * Nx);
            EXPECT_EQ(cr64Info[b].Count[0], Ny);
            EXPECT_EQ(cr64Info[b].Count[1], Nx);
            EXPECT_EQ(cr64Info[b].WriterID, inRank);

            const size_t posFrom = blockPosY * Ny * Nx;
            const size_t posTo = posFrom + Ny * Nx;

            int8_t i8Min, i8Max;
            int16_t i16Min, i16Max;
            int32_t i32Min, i32Max;
            int64_t i64Min, i64Max;
            uint8_t u8Min, u8Max;
            uint16_t u16Min, u16Max;
            uint32_t u32Min, u32Max;
            uint64_t u64Min, u64Max;
            float r32Min, r32Max;
            double r64Min, r64Max;
            std::complex<float> cr32Min, cr32Max;
            std::complex<double> cr64Min, cr64Max;

            i8Min = *std::min_element(currentTestData.I8.begin() + posFrom,
                                      currentTestData.I8.begin() + posTo);
            i8Max = *std::max_element(currentTestData.I8.begin() + posFrom,
                                      currentTestData.I8.begin() + posTo);
            i16Min = *std::min_element(currentTestData.I16.begin() + posFrom,
                                       currentTestData.I16.begin() + posTo);
            i16Max = *std::max_element(currentTestData.I16.begin() + posFrom,
                                       currentTestData.I16.begin() + posTo);
            i32Min = *std::min_element(currentTestData.I32.begin() + posFrom,
                                       currentTestData.I32.begin() + posTo);
            i32Max = *std::max_element(currentTestData.I32.begin() + posFrom,
                                       currentTestData.I32.begin() + posTo);
            i64Min = *std::min_element(currentTestData.I64.begin() + posFrom,
                                       currentTestData.I64.begin() + posTo);
            i64Max = *std::max_element(currentTestData.I64.begin() + posFrom,
                                       currentTestData.I64.begin() + posTo);
            u8Min = *std::min_element(currentTestData.U8.begin() + posFrom,
                                      currentTestData.U8.begin() + posTo);
            u8Max = *std::max_element(currentTestData.U8.begin() + posFrom,
                                      currentTestData.U8.begin() + posTo);
            u16Min = *std::min_element(currentTestData.U16.begin() + posFrom,
                                       currentTestData.U16.begin() + posTo);
            u16Max = *std::max_element(currentTestData.U16.begin() + posFrom,
                                       currentTestData.U16.begin() + posTo);
            u32Min = *std::min_element(currentTestData.U32.begin() + posFrom,
                                       currentTestData.U32.begin() + posTo);
            u32Max = *std::max_element(currentTestData.U32.begin() + posFrom,
                                       currentTestData.U32.begin() + posTo);
            u64Min = *std::min_element(currentTestData.U64.begin() + posFrom,
                                       currentTestData.U64.begin() + posTo);
            u64Max = *std::max_element(currentTestData.U64.begin() + posFrom,
                                       currentTestData.U64.begin() + posTo);
            r32Min = *std::min_element(currentTestData.R32.begin() + posFrom,
                                       currentTestData.R32.begin() + posTo);
            r32Max = *std::max_element(currentTestData.R32.begin() + posFrom,
                                       currentTestData.R32.begin() + posTo);
            r64Min = *std::min_element(currentTestData.R64.begin() + posFrom,
                                       currentTestData.R64.begin() + posTo);
            r64Max = *std::max_element(currentTestData.R64.begin() + posFrom,
                                       currentTestData.R64.begin() + posTo);

            cr32Min = currentTestData.CR32[posFrom];
            cr32Max = currentTestData.CR32[posFrom];
            for (auto it = currentTestData.CR32.begin() + posFrom;
                 it != currentTestData.CR32.begin() + posTo; ++it)
            {
                if (std::norm(*it) < std::norm(cr32Min))
                {
                    cr32Min = *it;
                    continue;
                }
                if (std::norm(*it) > std::norm(cr32Max))
                {
                    cr32Max = *it;
                }
            }

            cr64Min = currentTestData.CR64[posFrom];
            cr64Max = currentTestData.CR64[posFrom];
            for (auto it = currentTestData.CR64.begin() + posFrom;
                 it != currentTestData.CR64.begin() + posTo; ++it)
            {
                if (std::norm(*it) < std::norm(cr64Min))
                {
                    cr64Min = *it;
                    continue;
                }
                if (std::norm(*it) > std::norm(cr64Max))
                {
                    cr64Max = *it;
                }
            }

            EXPECT_EQ(i8Info[b].Min, i8Min);
            EXPECT_EQ(i8Info[b].Max, i8Max);
            EXPECT_EQ(i16Info[b].Min, i16Min);
            EXPECT_EQ(i16Info[b].Max, i16Max);
            EXPECT_EQ(i32Info[b].Min, i32Min);
            EXPECT_EQ(i32Info[b].Max, i32Max);
            EXPECT_EQ(i64Info[b].Min, i64Min);
            EXPECT_EQ(i64Info[b].Max, i64Max);

            EXPECT_EQ(u8Info[b].Min, u8Min);
            EXPECT_EQ(u8Info[b].Max, u8Max);
            EXPECT_EQ(u16Info[b].Min, u16Min);
            EXPECT_EQ(u16Info[b].Max, u16Max);
            EXPECT_EQ(u32Info[b].Min, u32Min);
            EXPECT_EQ(u32Info[b].Max, u32Max);
            EXPECT_EQ(u64Info[b].Min, u64Min);
            EXPECT_EQ(u64Info[b].Max, u64Max);

            EXPECT_EQ(r32Info[b].Min, r32Min);
            EXPECT_EQ(r32Info[b].Max, r32Max);
            EXPECT_EQ(r64Info[b].Min, r64Min);
            EXPECT_EQ(r64Info[b].Max, r64Max);

            EXPECT_EQ(cr32Info[b].Min, cr32Min);
            EXPECT_EQ(cr32Info[b].Max, cr32Max);
            EXPECT_EQ(cr64Info[b].Min, cr64Min);
            EXPECT_EQ(cr64Info[b].Max, cr64Max);

            var_i8.SetSelection(sel1);
            bpReader.Get(var_i8, I8.data());

            var_i16.SetSelection(sel1);
            bpReader.Get(var_i16, I16.data());

            var_i32.SetSelection(sel1);
            bpReader.Get(var_i32, I32.data());

            var_i64.SetSelection(sel1);
            bpReader.Get(var_i64, I64.data());

            var_u8.SetSelection(sel1);
            bpReader.Get(var_u8, U8.data());

            var_u16.SetSelection(sel1);
            bpReader.Get(var_u16, U16.data());

            var_u32.SetSelection(sel1);
            bpReader.Get(var_u32, U32.data());

            var_u64.SetSelection(sel1);
            bpReader.Get(var_u64, U64.data());

            var_r32.SetSelection(sel1);
            bpReader.Get(var_r32, R32.data());

            var_r64.SetSelection(sel1);
            bpReader.Get(var_r64, R64.data());

            var_cr32.SetSelection(sel1);
            bpReader.Get(var_cr32, CR32.data());

            var_cr64.SetSelection(sel1);
            bpReader.Get(var_cr64, CR64.data());

            bpReader.PerformGets();

            for (size_t i = 0; i < Nx * Ny; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " block=" << b;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], currentTestData.I8[i + posFrom]) << msg;
                EXPECT_EQ(I16[i], currentTestData.I16[i + posFrom]) << msg;
                EXPECT_EQ(I32[i], currentTestData.I32[i + posFrom]) << msg;
                EXPECT_EQ(I64[i], currentTestData.I64[i + posFrom]) << msg;
                EXPECT_EQ(U8[i], currentTestData.U8[i + posFrom]) << msg;
                EXPECT_EQ(U16[i], currentTestData.U16[i + posFrom]) << msg;
                EXPECT_EQ(U32[i], currentTestData.U32[i + posFrom]) << msg;
                EXPECT_EQ(U64[i], currentTestData.U64[i + posFrom]) << msg;
                EXPECT_EQ(R32[i], currentTestData.R32[i + posFrom]) << msg;
                EXPECT_EQ(R64[i], currentTestData.R64[i + posFrom]) << msg;
                EXPECT_EQ(CR32[i], currentTestData.CR32[i + posFrom]) << msg;
                EXPECT_EQ(CR64[i], currentTestData.CR64[i + posFrom]) << msg;
            }
        }

        bpReader.EndStep();
        ++t;
    }
    EXPECT_EQ(t, NSteps);
    bpReader.Close();
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc < 3)
    {
        throw std::runtime_error(
            "Test needs 2 arguments: path-to-data  engineName");
    }

    dataPath = std::string(argv[1]);
    engineName = std::string(argv[2]);
    result = RUN_ALL_TESTS();

    return result;
}
