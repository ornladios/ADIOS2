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

class ADIOSHierarchicalReadVariableTest : public ::testing::Test
{
public:
    ADIOSHierarchicalReadVariableTest() = default;
};

TEST_F(ADIOSHierarchicalReadVariableTest, Read)
{
    std::string filename = "ADIOSSelectSteps0.bp";

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
        const adios2::Dims shape = {10};
        const adios2::Dims start = {0};
        const adios2::Dims count = {10};

        auto var0 = ioWrite.DefineVariable<int32_t>(
                "variable0", shape, start, count);
        auto var1 = ioWrite.DefineVariable<int32_t>(
                "variable1", shape, start, count);
        auto var2 = ioWrite.DefineVariable<int32_t>(
                "variable2", shape, start, count);
        auto var3 = ioWrite.DefineVariable<int32_t>(
                "variable3", shape, start, count);

        std::vector<int32_t> Ints0 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        std::vector<int32_t> Ints1 = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
        std::vector<int32_t> Ints2 = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
        std::vector<int32_t> Ints3 = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
        for (size_t step = 0; step < NSteps; ++step)
        {
            engine.BeginStep();
            if (step == 0){
                engine.Put(var0, Ints0.data());
            }else if (step == 1){
                engine.Put(var1, Ints1.data());
            }else if (step == 2){
                engine.Put(var2, Ints2.data());
            }else if (step == 3){
                engine.Put(var3, Ints3.data());
            }

            engine.EndStep();
        }
        engine.Close();

#if ADIOS2_USE_MPI
        MPI_Barrier(MPI_COMM_WORLD);
#endif

        adios2::IO ioRead = adios.DeclareIO("TestIORead");
        ioRead.SetEngine("Filestream");
        ioRead.SetParameter(filename, "1,3");
        adios2::Engine engine_s = ioRead.Open(filename, adios2::Mode::Read);
        EXPECT_TRUE(engine_s);
        {

            for (int step = 0; step < NSteps; step++) {
                engine_s.BeginStep();
                adios2::Variable<int> var0 = ioRead.InquireVariable<int>("variable0");
                adios2::Variable<int> var1 = ioRead.InquireVariable<int>("variable1");
                adios2::Variable<int> var2 = ioRead.InquireVariable<int>("variable2");
                adios2::Variable<int> var3 = ioRead.InquireVariable<int>("variable3");
                if (var0) {
                    std::vector<int> res;
                    const std::size_t Nx = 10;
                    var0.SetSelection({{Nx * mpiRank},
                                       {Nx}});
                    engine_s.Get<int>(var0, res, adios2::Mode::Sync);
                    EXPECT_NE(res, Ints0);
                }
                if (var1) {
                    std::vector<int> res;
                    const std::size_t Nx = 10;
                    var1.SetSelection({{Nx * mpiRank},
                                       {Nx}});
                    engine_s.Get<int>(var1, res, adios2::Mode::Sync);
                    EXPECT_EQ(res, Ints1);
                }
                if (var2) {
                    std::vector<int> res;
                    const std::size_t Nx = 10;
                    var2.SetSelection({{Nx * mpiRank},
                                       {Nx}});
                    engine_s.Get<int>(var2, res, adios2::Mode::Sync);
                    EXPECT_NE(res, Ints2);
                }
                if (var3) {
                    std::vector<int> res;
                    const std::size_t Nx = 10;
                    var3.SetSelection({{Nx * mpiRank},
                                       {Nx}});
                    engine_s.Get<int>(var3, res, adios2::Mode::Sync);
                    EXPECT_EQ(res, Ints3);
                }


                engine_s.EndStep();
            }
        }
        engine_s.Close();
#if ADIOS2_USE_MPI
        MPI_Barrier(MPI_COMM_WORLD);
#endif

    }
}
int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    MPI_Init(nullptr, nullptr);
#endif
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
}
