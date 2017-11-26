
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
        adios2::ADIOS ad("adios2.xml", mpiReaderComm, adios2::DebugOFF);

        // Define method for engine creation
        // 1. Get method def from config file or define new one

        adios2::IO &h5ReaderIO = ad.DeclareIO("input");
        if (!h5ReaderIO.InConfigFile())
        {
            // if not defined by user, we can change the default settings
            // BPFileWriter is the default engine
            // h5ReaderIO.SetEngine("ADIOS1Reader");
            h5ReaderIO.SetEngine("HDF5Reader");
            // h5ReaderIO.SetParameters({{"num_threads", "2"}});

            // ISO-POSIX file is the default transport
            // Passing parameters to the transport
            h5ReaderIO.AddTransport("File", {{"verbose", "4"}});
        }

        adios2::Engine &h5Reader =
            h5ReaderIO.Open(inputfile, adios2::Mode::Read, mpiReaderComm);

        unsigned int gndx;
        unsigned int gndy;
        // h5Reader->Read<unsigned int>("gndx", &gndx);
        // h5Reader->Read<unsigned int>("gndy", &gndy);

        adios2::Variable<unsigned int> *vgndx =
            h5ReaderIO.InquireVariable<unsigned int>("gndx");

        // gndx = vgndx->GetData()[0];
        adios2::Variable<unsigned int> *vgndy =
            h5ReaderIO.InquireVariable<unsigned int>("gndy");
        // gndy = vgndy->GetData()[0];
        adios2::Variable<double> *vT = h5ReaderIO.InquireVariable<double>("T");

        if ((vgndx == NULL) || (vgndy == NULL) || (vT == NULL))
        {
            std::cout << "Unable to find expected variables: gndx, gndy and T"
                      << std::endl;
            return 1;
        }

        h5Reader.GetSync<unsigned int>(*vgndx, gndx);
        h5Reader.GetSync<unsigned int>(*vgndy, gndy);

        if (rank == 0)
        {
            std::cout << "gndx       = " << gndx << std::endl;
            std::cout << "gndy       = " << gndy << std::endl;
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

        if (readsize[1] == 0)
        {
            std::cout << "Nothing to read. exiting" << std::endl;
            return 0;
        }

        double *T = new double[vT->GetAvailableStepsCount() * readsize[0] *
                               readsize[1]];

        // Create a 2D selection for the subset
        vT->SetSelection(adios2::Box<adios2::Dims>(offset, readsize));
        vT->SetStepSelection(
            adios2::Box<std::size_t>(0, vT->GetAvailableStepsCount()));

        h5Reader.GetSync<double>(*vT, T);
        // Arrays are read by scheduling one or more of them
        // and performing the reads at once
        // h5Reader->ScheduleRead<double>(*vT, T);
        // h5Reader->PerformReads(adios2::ReadMode::Blocking);

        printData(T, readsize.data(), offset.data(), rank,
                  vT->GetAvailableStepsCount());
        h5Reader.Close();
        delete[] T;
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "IO System base failure exception, STOPPING PROGRAM "
                     "from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }

    MPI_Finalize();
    return 0;
}
