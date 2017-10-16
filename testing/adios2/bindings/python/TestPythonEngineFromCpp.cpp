/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include <adios2.h>
#include <gtest/gtest.h>

/**
 * Test that with the proper plugin module and class specified, we
 * can successfully create the python engine from C++.
 */
TEST(PythonEngineTest, CreatePythonEngineFromCPlusPlus)
{
    int mpiRank = 0, mpiSize = 1;

    // Application variable
    std::vector<double> myDoubles = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myDoubles.size();

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(adios2::DebugON);
#endif

    adios2::IO &io = adios.DeclareIO("PythonPluginIO");

    adios2::Variable<double> &var = io.DefineVariable<double>(
        "data", {mpiSize * Nx}, {mpiRank * Nx}, {Nx}, adios2::ConstantDims);

    io.SetEngine("PythonEngine");
    io.SetParameters({{"PluginName", "TestPythonPlugin"},
                      {"PluginModule", "TestPythonEngine"},
                      {"PluginClass", "TestPythonEngine"}});

    std::shared_ptr<adios2::Engine> writer;
    EXPECT_NO_THROW(
        { writer = io.Open("TestPythonPlugin", adios2::OpenMode::Write); });

    ASSERT_NE(writer.get(), nullptr);

    /** Write variable for buffering */
    writer->Write<double>(var, myDoubles.data());

    writer->Close();
}

/**
 * Test that without the proper plugin module specified, IO fails to
 * instantiate the engine and throws the correct exception type.
 */
TEST(PythonEngineTest, CreatePythonEngineFromCPlusPlusExpectImportError)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(adios2::DebugON);
#endif

    adios2::IO &io = adios.DeclareIO("PythonPluginIO");

    io.SetEngine("PythonEngine");

    // By not specifying "PluginModule" parameter, we should fail during
    // the "Open" call...
    io.SetParameters({{"PluginName", "TestPythonPlugin"},
                      {"PluginClass", "TestPythonEngine"}});

    std::shared_ptr<adios2::Engine> writer;

    ASSERT_THROW(
        { writer = io.Open("TestPythonPlugin", adios2::OpenMode::Write); },
        std::runtime_error);
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
