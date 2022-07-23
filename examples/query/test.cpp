#include "adios2.h"
#include <mpi.h>

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
    int provided;

    // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    try
    {
        std::string configFileName = "query.xml";
        std::string dataFileName = "/tmp/heatbp4.bp";
        if (argc <= 2)
        {
            std::cout << "Usage: " << argv[0]
                      << " configFileName  dataFilePath (queryFile)"
                      << std::endl;
            std::cout << "    e.g.  " << argv[0] << " bp4io.xml heat_bp4.bp/ "
                      << std::endl;
            std::cout << "    or    " << argv[0]
                      << " bp4io.xml heat_bp4.bp/ q1.json" << std::endl;
            return 0;
        }

        configFileName = argv[1];
        dataFileName = argv[2];

        if (rank == 0)
        {
            std::cout << " using config file = " << configFileName << std::endl;
            std::cout << "        data file  = " << dataFileName << std::endl;
        }

        adios2::ADIOS ad = adios2::ADIOS(configFileName, MPI_COMM_WORLD);

        adios2::IO queryIO = ad.DeclareIO("query");
        adios2::Engine reader =
            queryIO.Open(dataFileName, adios2::Mode::Read, MPI_COMM_WORLD);
#ifdef NEVER
        adios2::QueryWorker w = adios2::QueryWorker(configFileName, reader);
#else
        std::string queryFile = configFileName;
        if (argc > 3)
        {
            queryFile = argv[3];
        }
        std::cout << "Testing query file  ..." << queryFile << std::endl;
        adios2::QueryWorker w = adios2::QueryWorker(queryFile, reader);
#endif
        std::vector<adios2::Box<adios2::Dims>> touched_blocks;

        while (reader.BeginStep() == adios2::StepStatus::OK)
        {
            adios2::Box<adios2::Dims> empty;
            w.GetResultCoverage(empty, touched_blocks);
            // adios2::Box<adios2::Dims> tt({10,10}, {12,12});
            // w.GetResultCoverage(tt, touched_blocks);
            std::cout << " ... now can read out touched blocks ... size="
                      << touched_blocks.size() << std::endl;
            for (auto n : touched_blocks)
            {
                std::ostringstream startStr;
                std::ostringstream countStr;
                for (size_t k = 0; k < n.first.size(); k++)
                {
                    startStr << n.first[k] << " ";
                    countStr << n.second[k] << " ";
                }
                std::cout << "\t[" << startStr.str() << "]  [" << countStr.str()
                          << "]" << std::endl;
            }
            reader.EndStep();
        }
        reader.Close();
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
    adios2::ADIOS ad(queryConfigFile, comm);
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

    //  to be continued
    if (!reader)
        return false;
    // std::vector<double> dataOutput;
    // std::vector<adios2::Dims> coordinateOutput;

    return true;
}

/*
 */
