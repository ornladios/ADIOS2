#include <cstdint>
#include <cstring>

#include <iostream>
#include <numeric> //std::iota
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

std::string engineName;       // comes from command line
std::string engineParameters; // comes from command line

class BPExtremeVars : public ::testing::Test
{
public:
    BPExtremeVars() = default;
};

#include <adios2.h>
#include <iomanip>
#include <iostream>

std::string createVariable(adios2::IO &io, int counter, int rank, long unsigned int bufferSize)
{
    std::stringstream sStream;
    sStream << "Var-" << std::setfill('0') << std::setw(3) << counter << std::setw(3) << "_Rank-"
            << rank;
    adios2::Variable<double> var = io.DefineVariable<double>(sStream.str(), {}, {}, {bufferSize});
    return sStream.str();
}

int numVarTotal = -1;
int numWrittenVariables = 0;
TEST_F(BPExtremeVars, WriteRead)
{
    const std::string fname("ExtremeVars");
    int commSize = 1;
    int commRank = 0;
#if ADIOS2_USE_MPI
    MPI_Comm_size(MPI_COMM_WORLD, &commSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &commRank);
#endif

    // -------------
    //  ADIOS setup
    // -------------
#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif
    {
        adios2::IO output = adios.DeclareIO("Writer");
        output.SetEngine(engineName);
        adios2::Engine writer = output.Open(fname, adios2::Mode::Write);
        int numVarLocal = numVarTotal / commSize;

        int counter = 0;

        unsigned long int bufferSize = 100u;
        std::vector<double> buffer(bufferSize, 2.0);

        for (int k = 1; k <= numVarLocal; ++k)
        {
            double *address = buffer.data();
            std::string var = createVariable(output, counter++, commRank, bufferSize);
            writer.Put<double>(var, address);
        }

        numWrittenVariables = counter;
        MPI_Reduce(&counter, &numWrittenVariables, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        if (commRank == 0)
        {
            std::cout << "Total variables created = " << numWrittenVariables << std::endl;
        }
        writer.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::ReadRandomAccess);

        EXPECT_EQ(bpReader.Steps(), 1);
        auto vars = io.AvailableVariables();
        if (commRank == 0)
        {
            EXPECT_EQ(vars.size(), numWrittenVariables);
        }
        bpReader.Close();
    }
}
//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    int provided;

    // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    numVarTotal = 20000;

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    if (argc > 2)
    {
        engineParameters = std::string(argv[2]);
    }
    result = RUN_ALL_TESTS();

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
}
