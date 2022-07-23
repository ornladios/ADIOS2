/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <limits>
#include <stdexcept>

#include <adios2.h>
#include <adios2/common/ADIOSTypes.h>

#include <gtest/gtest.h>

class ADIOSReadSelectionStepsTest : public ::testing::Test
{
public:
    ADIOSReadSelectionStepsTest() = default;
};

TEST_F(ADIOSReadSelectionStepsTest, Read)
{
    std::string filename = "ADIOSSelectSteps.bp";

    // Number of steps
    const std::size_t NSteps = 4;
    int mpiRank = 0, mpiSize = 1;

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    // Write test data using BP
    {
#if ADIOS2_USE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD);
#else
        adios2::ADIOS adios;
#endif
        adios2::IO ioWrite = adios.DeclareIO("TestIOWrite");
        ioWrite.SetEngine("BPFile");

        adios2::Engine engine = ioWrite.Open(filename, adios2::Mode::Write);
        // Number of elements per process
        const std::size_t Nx = 10;
        adios2::Dims shape{static_cast<unsigned int>(mpiSize * Nx)};
        adios2::Dims start{static_cast<unsigned int>(mpiRank * Nx)};
        adios2::Dims count{static_cast<unsigned int>(Nx)};

        auto var0 =
            ioWrite.DefineVariable<int32_t>("variable0", shape, start, count);
        auto var1 =
            ioWrite.DefineVariable<int32_t>("variable1", shape, start, count);
        auto var2 =
            ioWrite.DefineVariable<int32_t>("variable2", shape, start, count);
        auto var3 =
            ioWrite.DefineVariable<int32_t>("variable3", shape, start, count);

        std::vector<int32_t> Ints0 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        std::vector<int32_t> Ints1 = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
        std::vector<int32_t> Ints2 = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
        std::vector<int32_t> Ints3 = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
        for (size_t step = 0; step < NSteps; ++step)
        {
            engine.BeginStep();
            engine.Put(var0, Ints0.data());
            engine.Put(var1, Ints1.data());
            engine.Put(var2, Ints2.data());
            engine.Put(var3, Ints3.data());
            engine.EndStep();
        }
        engine.Close();

#if ADIOS2_USE_MPI
        MPI_Barrier(MPI_COMM_WORLD);
#endif
        adios2::IO ioRead = adios.DeclareIO("TestIORead");
        ioRead.SetEngine("Filestream");
        ioRead.SetParameter(filename, "1,3");
        ioRead.SetParameter("StreamReader", "On");
        adios2::Engine engine_s = ioRead.Open(filename, adios2::Mode::Read);
        EXPECT_TRUE(engine_s);
        try
        {
            for (size_t step = 0; step < NSteps; step++)
            {
                engine_s.BeginStep();
                adios2::Variable<int> var0 =
                    ioRead.InquireVariable<int>("variable0");
                adios2::Variable<int> var1 =
                    ioRead.InquireVariable<int>("variable1");
                adios2::Variable<int> var2 =
                    ioRead.InquireVariable<int>("variable2");
                adios2::Variable<int> var3 =
                    ioRead.InquireVariable<int>("variable3");

                if (step == 0)
                {
                    EXPECT_FALSE(var0);
                    EXPECT_FALSE(var1);
                    EXPECT_FALSE(var2);
                    EXPECT_FALSE(var3);
                }
                else if (step == 1)
                {
                    EXPECT_TRUE(var0);
                    EXPECT_TRUE(var1);
                    EXPECT_TRUE(var2);
                    EXPECT_TRUE(var3);
                }
                else if (step == 2)
                {
                    EXPECT_FALSE(var0);
                    EXPECT_FALSE(var1);
                    EXPECT_FALSE(var2);
                    EXPECT_FALSE(var3);
                }
                else if (step == 3)
                {
                    EXPECT_TRUE(var0);
                    EXPECT_TRUE(var1);
                    EXPECT_TRUE(var2);
                    EXPECT_TRUE(var3);
                }
                if (var0)
                {
                    std::vector<int> res;
                    var0.SetSelection({{Nx * mpiRank}, {Nx}});
                    engine_s.Get<int>(var0, res, adios2::Mode::Sync);
                    EXPECT_EQ(res, Ints0);
                }
                if (var1)
                {
                    std::vector<int> res;
                    var1.SetSelection({{Nx * mpiRank}, {Nx}});
                    engine_s.Get<int>(var1, res, adios2::Mode::Sync);
                    EXPECT_EQ(res, Ints1);
                }
                if (var2)
                {
                    std::vector<int> res;
                    var2.SetSelection({{Nx * mpiRank}, {Nx}});
                    engine_s.Get<int>(var2, res, adios2::Mode::Sync);
                    EXPECT_EQ(res, Ints2);
                }
                if (var3)
                {
                    std::vector<int> res;
                    var3.SetSelection({{Nx * mpiRank}, {Nx}});
                    engine_s.Get<int>(var3, res, adios2::Mode::Sync);
                    EXPECT_EQ(res, Ints3);
                }
                engine_s.EndStep();
            }
        }
        catch (std::exception &e)
        {
            std::cout << "Exception " << e.what() << std::endl;
        }
        engine_s.Close();
#if ADIOS2_USE_MPI
        MPI_Barrier(MPI_COMM_WORLD);
#endif
        adios2::IO ioReadBP = adios.DeclareIO("ReadBP");
        ioReadBP.SetEngine("BPfile");
        ioReadBP.SetParameter(filename, "1,3");
        /** Engine derived class, spawned to start IO operations */
        adios2::Engine engine_b = ioReadBP.Open(filename, adios2::Mode::Read);
        EXPECT_TRUE(engine_b);

        try
        {
            for (size_t step = 0; step < NSteps; ++step)
            {
                engine_b.BeginStep();
                adios2::Variable<int> var0 =
                    ioReadBP.InquireVariable<int>("variable0");
                adios2::Variable<int> var1 =
                    ioReadBP.InquireVariable<int>("variable1");
                adios2::Variable<int> var2 =
                    ioReadBP.InquireVariable<int>("variable2");
                adios2::Variable<int> var3 =
                    ioReadBP.InquireVariable<int>("variable3");

                /**  Variables are not updated */
                /**  EXPECT_EQ(var0.Steps(), 2); */
                /**  EXPECT_EQ(engine_b.GetAbsoluteSteps(var0), {1,3}); */
                if (step == 0)
                {
                    EXPECT_FALSE(var0);
                    EXPECT_FALSE(var1);
                    EXPECT_FALSE(var2);
                    EXPECT_FALSE(var3);
                }
                else if (step == 1)
                {
                    EXPECT_TRUE(var0);
                    EXPECT_TRUE(var1);
                    EXPECT_TRUE(var2);
                    EXPECT_TRUE(var3);
                }
                else if (step == 2)
                {
                    EXPECT_FALSE(var0);
                    EXPECT_FALSE(var1);
                    EXPECT_FALSE(var2);
                    EXPECT_FALSE(var3);
                }
                else if (step == 3)
                {
                    EXPECT_TRUE(var0);
                    EXPECT_TRUE(var1);
                    EXPECT_TRUE(var2);
                    EXPECT_TRUE(var3);
                }
                if (var0)
                {
                    std::vector<int> res;
                    var0.SetSelection({{Nx * mpiRank}, {Nx}});
                    engine_b.Get<int>(var0, res, adios2::Mode::Sync);
                    EXPECT_EQ(res, Ints0);
                }
                if (var1)
                {
                    std::vector<int> res;
                    var1.SetSelection({{Nx * mpiRank}, {Nx}});
                    engine_b.Get<int>(var1, res, adios2::Mode::Sync);
                    EXPECT_EQ(res, Ints1);
                }
                if (var2)
                {
                    std::vector<int> res;
                    var2.SetSelection({{Nx * mpiRank}, {Nx}});
                    engine_b.Get<int>(var2, res, adios2::Mode::Sync);
                    EXPECT_EQ(res, Ints2);
                }
                if (var3)
                {
                    std::vector<int> res;
                    var3.SetSelection({{Nx * mpiRank}, {Nx}});
                    engine_b.Get<int>(var3, res, adios2::Mode::Sync);
                    EXPECT_EQ(res, Ints3);
                }
                engine_b.EndStep();
            }
        }
        catch (std::exception &e)
        {
            std::cout << "Exception " << e.what() << std::endl;
        }
        engine_b.Close();

#if ADIOS2_USE_MPI
        MPI_Barrier(MPI_COMM_WORLD);
#endif
        adios2::IO ioReadBPFull = adios.DeclareIO("ReadBPFull");
        ioReadBPFull.SetParameter(filename, "1,3");
        /** Engine derived class, spawned to start IO operations */
        adios2::Engine engine_bf =
            ioReadBPFull.Open(filename, adios2::Mode::Read);

        EXPECT_TRUE(engine_bf);
        {
            adios2::Variable<int> var0 =
                ioReadBPFull.InquireVariable<int>("variable0");
            adios2::Variable<int> var1 =
                ioReadBPFull.InquireVariable<int>("variable1");
            adios2::Variable<int> var2 =
                ioReadBPFull.InquireVariable<int>("variable2");
            adios2::Variable<int> var3 =
                ioReadBPFull.InquireVariable<int>("variable3");
            /**  Variables are not updated */
            /**  EXPECT_EQ(var0.Steps(), 2); */
            /**  EXPECT_EQ(engine_b.GetAbsoluteSteps(var0), {1,3}); */
            EXPECT_TRUE(var0);
            EXPECT_TRUE(var1);
            EXPECT_TRUE(var2);
            EXPECT_TRUE(var3);
            {
                std::vector<int> res;
                var0.SetStepSelection({1, 1});
                var0.SetSelection({{Nx * mpiRank}, {Nx}});
                engine_bf.Get<int>(var0, res, adios2::Mode::Sync);
                EXPECT_EQ(res, Ints0);
            }
            {
                std::vector<int> res;
                var1.SetStepSelection({1, 1});
                var1.SetSelection({{Nx * mpiRank}, {Nx}});
                engine_bf.Get<int>(var1, res, adios2::Mode::Sync);
                EXPECT_EQ(res, Ints1);
            }

            {
                std::vector<int> res;
                var2.SetStepSelection({1, 1});
                var2.SetSelection({{Nx * mpiRank}, {Nx}});
                engine_bf.Get<int>(var2, res, adios2::Mode::Sync);
                EXPECT_EQ(res, Ints2);
            }

            {
                std::vector<int> res;
                var3.SetStepSelection({1, 1});
                var3.SetSelection({{Nx * mpiRank}, {Nx}});
                engine_bf.Get<int>(var3, res, adios2::Mode::Sync);
                EXPECT_EQ(res, Ints3);
            }
        }
        engine_bf.Close();
    }
}

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    int provided;

    // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
#endif
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
}
