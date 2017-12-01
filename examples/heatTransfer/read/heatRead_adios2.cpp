
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

    try
    {
        // adios2::ADIOS ad("adios2.xml", mpiReaderComm);
        adios2::ADIOS ad(mpiReaderComm);

        // Define method for engine creation
        // 1. Get method def from config file or define new one

        adios2::IO &bpReaderIO = ad.DeclareIO("input");
        if (!bpReaderIO.InConfigFile())
        {
            // if not defined by user, we can change the default settings
            // BPFileWriter is the default engine
            // bpReaderIO.SetEngine("ADIOS1Reader");
            bpReaderIO.SetParameters({{"Threads", "2"}});

            // ISO-POSIX file is the default transport
            // Passing parameters to the transport
            // bpReaderIO.AddTransport("File", {{"verbose", "4"}});
        }

        adios2::Engine &bpReader =
            bpReaderIO.Open(inputfile, adios2::Mode::Read, mpiReaderComm);

        unsigned int gndx = 0;
        unsigned int gndy = 0;
        // bpReader->Read<unsigned int>("gndx", &gndx);
        // bpReader->Read<unsigned int>("gndy", &gndy);

        adios2::Variable<unsigned int> *vgndx =
            bpReaderIO.InquireVariable<unsigned int>("gndx");

        if (vgndx != nullptr && vgndx->m_SingleValue)
        {
            gndx = vgndx->m_Value;
        }

        adios2::Variable<unsigned int> *vgndy =
            bpReaderIO.InquireVariable<unsigned int>("gndy");

        if (vgndy != nullptr && vgndy->m_SingleValue)
        {
            gndy = vgndy->m_Value;
        }

        if (rank == 0)
        {
            std::cout << "gndx       = " << gndx << "\n";
            std::cout << "gndy       = " << gndy << "\n";
            std::cout << "# of steps = " << vgndy->GetAvailableStepsCount()
                      << std::endl;
        }

        // 1D decomposition of the columns, which is inefficient for reading!
        adios2::Dims readsize({gndx, gndy / nproc});
        adios2::Dims offset({0LL, rank * readsize[1]});
        if (rank == nproc - 1)
        {
            // last process should read all the rest of columns
            readsize[1] = gndy - readsize[1] * (nproc - 1);
        }

        std::cout << "rank " << rank << " reads " << readsize[1]
                  << " columns from offset " << offset[1] << std::endl;

        adios2::Variable<double> *vT = bpReaderIO.InquireVariable<double>("T");
        if (vT == nullptr)
        {
            throw std::runtime_error("T variable not found\n");
        }

        std::vector<double> T(vT->GetAvailableStepsCount() * readsize[0] *
                              readsize[1]);

        // Create a 2D selection for the subset
        vT->SetSelection(adios2::Box<adios2::Dims>(offset, readsize));
        vT->SetStepSelection(
            adios2::Box<std::size_t>(0, vT->GetAvailableStepsCount()));

        bpReader.GetSync(*vT, T.data());

        printData(T.data(), readsize.data(), offset.data(), rank,
                  vT->GetAvailableStepsCount());
        bpReader.Close();
    }
    catch (std::exception &e)
    {
        std::cout << "Caught exception from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }

    MPI_Finalize();
    return 0;
}
