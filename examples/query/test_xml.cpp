#include <mpi.h>

#include "adios2.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

//#include "adios2/toolkit/query/Worker.h"

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const std::size_t Nx = 10;

    try
    {
        // size_t recommendedSize = 20000;
        // bool overwrite = false;

        std::string configFileName = "query.xml";
        std::string dataFileName = "/tmp/heatbp4.bp";
        if (argc > 1)
            configFileName = argv[1];

        if (argc > 2)
            dataFileName = argv[2];

        if (rank == 0)
        {
            std::cout << " file = " << configFileName << std::endl;
        }

        adios2::ADIOS ad =
            adios2::ADIOS(configFileName, MPI_COMM_WORLD, adios2::DebugON);

        adios2::IO queryIO = ad.DeclareIO("query");
        adios2::Engine reader =
            queryIO.Open(dataFileName, adios2::Mode::Read, MPI_COMM_WORLD);
        adios2::QueryWorker w = adios2::QueryWorker(configFileName, reader);
        // adios2::query::Worker* w = adios2::query::GetWorker(configFileName,
        // MPI_COMM_WORLD);

        std::vector<adios2::Box<adios2::Dims>> touched_blocks;

        // adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
        // adios2::IO bpIO = adios.DeclareIO("QueryTest");
        // bpIO.SetEngine("BPFile");
        // adios2::Engine reader =  bpIO.Open(dataFileName, adios2::Mode::Read);

        while (reader.BeginStep() == adios2::StepStatus::OK)
        {
            adios2::Box<adios2::Dims> empty;
            w.GetResultCoverage(empty, touched_blocks);
            std::cout << " ... use reader to read out touched blocks ... size="
                      << touched_blocks.size() << std::endl;
            reader.EndStep();
        }

        return 0;
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }

    MPI_Finalize();

    return 0;
}

bool testMe(std::string &queryConfigFile, std::string const &doubleVarName,
            MPI_Comm comm)
{
    adios2::ADIOS ad(queryConfigFile, comm, adios2::DebugON);
    std::string dataFileName = "test.file";

    // adios2::query::Worker w(queryConfigFile, comm);

    // w.SetSource(inIO, reader);

    {
        // the config file should have info on idx method to use

        /*
        // if wanting to build customized minmax idx
        // instead of using the existing block stats or bp4 minmax arrays
        bool overwrite = false;
        size_t recommendedSize = 20000;

        if (!w.PrepareIdx(overwrite, recommendedSize,  doubleVarName))
          return false;
        */
    }

    adios2::IO inIO = ad.DeclareIO("query");
    adios2::Engine reader = inIO.Open(dataFileName, adios2::Mode::Read, comm);

    // std::vector<double> dataOutput;
    // std::vector<adios2::Dims> coordinateOutput;

    return true;
}

/*
 */
