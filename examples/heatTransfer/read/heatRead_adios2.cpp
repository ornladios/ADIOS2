
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

#include "PrintData.h"

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    if (argc < 2)
    {
        std::cout << "Not enough arguments: need an input file\n";
        return 1;
    }
    const char *inputfile = argv[1];

    /* World comm spans all applications started with the same aprun command
     on a Cray XK6. So we have to split and create the local
     'world' communicator for the reader only.
     In normal start-up, the communicator will just equal the MPI_COMM_WORLD.
     */

    int wrank, wnproc;
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wnproc);
    MPI_Barrier(MPI_COMM_WORLD);

    const unsigned int color = 2;
    MPI_Comm mpiReaderComm;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &mpiReaderComm);

    int rank, nproc;
    MPI_Comm_rank(mpiReaderComm, &rank);
    MPI_Comm_size(mpiReaderComm, &nproc);

    adios::ADIOS ad("adios2.xml", mpiReaderComm, adios::Verbose::INFO);

    // Define method for engine creation
    // 1. Get method def from config file or define new one

    adios::Method &bpReaderSettings = ad.DeclareMethod("input");
    if (!bpReaderSettings.IsUserDefined())
    {
        // if not defined by user, we can change the default settings
        // BPFileWriter is the default engine
        bpReaderSettings.SetEngine("ADIOS1Reader");
        // Allow an extra thread for data processing
        bpReaderSettings.AllowThreads(1);
        // ISO-POSIX file is the default transport
        // Passing parameters to the transport
        bpReaderSettings.AddTransport("File", "verbose=4");
    }

    auto bpReader = ad.Open(inputfile, "r", mpiReaderComm, bpReaderSettings);

    if (bpReader == nullptr)
        throw std::ios_base::failure("ERROR: failed to open ADIOS bpReader\n");

    unsigned int gndx;
    unsigned int gndy;
    bpReader->Read<unsigned int>("gndx", &gndx);
    bpReader->Read<unsigned int>("gndy", &gndy);

    if (rank == 0)
    {
        std::cout << "gndx = " << gndx << std::endl;
        std::cout << "gndy = " << gndy << std::endl;
    }

    // 1D decomposition of the columns, which is inefficient for reading!
    std::vector<uint64_t> readsize({gndx, gndy / nproc});
    std::vector<uint64_t> offset({0LL, rank * readsize[1]});
    if (rank == nproc - 1)
    {
        // last process should read all the rest of columns
        readsize[1] = gndy - readsize[1] * (nproc - 1);
    }

    std::cout << "rank " << rank << " reads " << readsize[1]
              << " columns from offset " << offset[1] << std::endl;

    adios::Variable<double> *vT = bpReader->InquireVariableDouble("T");

    double *T = new double[vT->GetNSteps() * readsize[0] * readsize[1]];

    // Create a 2D selection for the subset
    adios::SelectionBoundingBox sel(offset, readsize);
    vT->SetSelection(sel);

    // Arrays are read by scheduling one or more of them
    // and performing the reads at once
    bpReader->ScheduleRead(*vT, T);
    bpReader->PerformReads(adios::PerformReadMode::BLOCKINGREAD);

    printData(T, readsize.data(), offset.data(), rank, vT->GetNSteps());
    bpReader->Close();
    delete[] T;
    MPI_Finalize();
    return 0;
}
