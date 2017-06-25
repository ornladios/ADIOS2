
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

    adios2::ADIOS ad("adios2.xml", mpiReaderComm, adios2::DebugON);

    // Define method for engine creation
    // 1. Get method def from config file or define new one

    adios2::IO &bpReaderIO = ad.DeclareIO("input");
    if (!bpReaderIO.InConfigFile())
    {
        // if not defined by user, we can change the default settings
        // BPFileWriter is the default engine
        bpReaderIO.SetEngine("ADIOS1Reader");
        bpReaderIO.SetParameters({{"num_threads", "2"}});

        // ISO-POSIX file is the default transport
        // Passing parameters to the transport
        bpReaderIO.AddTransport("File", {{"verbose", "4"}});
    }

    auto bpReader =
        bpReaderIO.Open(inputfile, adios2::OpenMode::Read, mpiReaderComm);

    if (!bpReader)
    {
        throw std::ios_base::failure("ERROR: failed to open " +
                                     std::string(inputfile) + "\n");
    }

    unsigned int gndx;
    unsigned int gndy;
    // bpReader->Read<unsigned int>("gndx", &gndx);
    // bpReader->Read<unsigned int>("gndy", &gndy);

    adios2::Variable<unsigned int> *vgndx =
        bpReader->InquireVariable<unsigned int>("gndx");

    gndx = vgndx->m_Data[0];

    adios2::Variable<unsigned int> *vgndy =
        bpReader->InquireVariable<unsigned int>("gndy");
    gndy = vgndy->m_Data[0];

    if (rank == 0)
    {
        std::cout << "gndx       = " << gndx << std::endl;
        std::cout << "gndy       = " << gndy << std::endl;
        std::cout << "# of steps = " << vgndy->m_AvailableSteps << std::endl;
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

    adios2::Variable<double> *vT = bpReader->InquireVariable<double>("T");

    double *T = new double[vT->m_AvailableSteps * readsize[0] * readsize[1]];

    // Create a 2D selection for the subset
    vT->SetSelection(offset, readsize);
    vT->SetStepSelection(0, vT->m_AvailableSteps);

    // Arrays are read by scheduling one or more of them
    // and performing the reads at once
    bpReader->ScheduleRead<double>(*vT, T);
    bpReader->PerformReads(adios2::ReadMode::Blocking);

    printData(T, readsize.data(), offset.data(), rank, vT->m_AvailableSteps);
    bpReader->Close();
    delete[] T;
    MPI_Finalize();
    return 0;
}
