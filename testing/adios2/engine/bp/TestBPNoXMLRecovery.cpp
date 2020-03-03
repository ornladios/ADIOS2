#include <iostream>
#include <vector>

#include <adios2.h>
#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

int main(int argc, char *argv[])
{
    int rank = 0;
#ifdef ADIOS2_HAVE_MPI
    int size = 1;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif
    adios2::ADIOS ad;
    try
    {
#ifdef ADIOS2_HAVE_MPI
        ad = adios2::ADIOS("does_not_exist.xml", MPI_COMM_WORLD,
                           adios2::DebugON);
#else
        ad = adios2::ADIOS("does_not_exist.xml", adios2::DebugON);
#endif
    }
    catch (std::exception &e)
    {
        if (rank == 0)
        {
            std::cout << e.what() << "\n";
        }
#ifdef ADIOS2_HAVE_MPI
        ad = adios2::ADIOS(MPI_COMM_WORLD, adios2::DebugON);
#else
        ad = adios2::ADIOS(adios2::DebugON);
#endif
    }

#ifdef ADIOS2_HAVE_MPI
    return MPI_Finalize();
#endif
}
