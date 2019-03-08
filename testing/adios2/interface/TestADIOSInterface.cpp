#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#ifdef ADIOS2_HAVE_MPI

TEST(ADIOSInterface, MPICommRemoved)
{
    MPI_Comm myComm;
    MPI_Comm_dup(MPI_COMM_WORLD, &myComm);
    adios2::ADIOS adios(myComm);
    adios2::IO io = adios.DeclareIO("TestIO");
    MPI_Comm_free(&myComm);

    adios2::Engine engine = io.Open("test.bp", adios2::Mode::Write);
}

#endif

class ADIOS2_CXX11_API : public ::testing::Test
{
public:
    ADIOS2_CXX11_API()
#ifdef ADIOS2_HAVE_MPI
    : ad(MPI_COMM_WORLD, adios2::DebugON)
#else
    : ad(adios2::DebugON)
#endif
    {
#ifdef ADIOS2_HAVE_MPI
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif
    }

    adios2::ADIOS ad;
    int rank = 0;
    int size = 1;
};

class ADIOS2_CXX11_API_IO : public ADIOS2_CXX11_API
{
public:
    ADIOS2_CXX11_API_IO() : io(ad.DeclareIO("CXX11_API_TestIO")) {}

    adios2::IO io;
};

TEST_F(ADIOS2_CXX11_API_IO, RemoveVariable)
{
    using T = int;

    auto var1 = io.DefineVariable<T>("var1");
    auto var2 = io.DefineVariable<T>("var2");
    io.RemoveVariable("var1");
    auto var3 = io.DefineVariable<T>("var3");
    EXPECT_EQ(var2.Name(), "var2");
    EXPECT_EQ(var3.Name(), "var3");
}

TEST_F(ADIOS2_CXX11_API_IO, Engine)
{
    io.SetEngine("bpfile");
    EXPECT_EQ(io.EngineType(), "bpfile");

    adios2::Engine engine = io.Open("types.bp", adios2::Mode::Write);
    EXPECT_EQ(engine.Name(), "types.bp");
    EXPECT_EQ(engine.Type(), "BP3");

    EXPECT_EQ(io.EngineType(), "bp"); // FIXME? Is it expected that adios2_open
                                      // changes the engine_type string?
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
