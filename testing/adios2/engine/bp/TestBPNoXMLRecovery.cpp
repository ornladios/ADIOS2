#include <iostream>
#include <vector>

#include <adios2.h>
#include <mpi.h>


int main(int argc, char *argv[])
{
    int rank = 0;
    int size = 1;
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif
    adios2::ADIOS ad;
    try
    {
        ad = adios2::ADIOS("does_not_exist.xml", MPI_COMM_WORLD, adios2::DebugON);
    }
    catch (std::exception &e)
    {
        if (rank == 0)
        {
            std::cout << e.what() << "\n";
        }
        ad = adios2::ADIOS(MPI_COMM_WORLD, adios2::DebugON);
    }

#ifdef ADIOS2_HAVE_MPI
    return MPI_Finalize();
#endif
}
