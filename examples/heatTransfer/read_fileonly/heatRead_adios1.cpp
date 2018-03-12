#include <cstdint>

#include <iomanip>
#include <iostream>
#include <math.h>
#include <memory>
#include <stdexcept>
#include <string>

#include <adios_read.h>
#include <mpi.h>

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

    const unsigned int color = 2;
    MPI_Comm mpiReaderComm;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &mpiReaderComm);

    int rank, nproc;
    MPI_Comm_rank(mpiReaderComm, &rank);
    MPI_Comm_size(mpiReaderComm, &nproc);

    adios_read_init_method(ADIOS_READ_METHOD_BP, mpiReaderComm, "verbose=3");

    ADIOS_FILE *f;
    f = adios_read_open_file(inputfile, ADIOS_READ_METHOD_BP, mpiReaderComm);
    if (f == NULL)
    {
        std::cout << adios_errmsg() << std::endl;
        return -1;
    }

    ADIOS_VARINFO *vgndx = adios_inq_var(f, "gndx");
    ADIOS_VARINFO *vgndy = adios_inq_var(f, "gndy");

    unsigned int gndx = *(unsigned int *)vgndx->value;
    unsigned int gndy = *(unsigned int *)vgndy->value;

    if (rank == 0)
    {
        std::cout << "gndx = " << gndx << std::endl;
        std::cout << "gndy = " << gndy << std::endl;
    }
    adios_free_varinfo(vgndx);
    adios_free_varinfo(vgndy);

    // 1D decomposition of the columns, which is inefficient for reading!
    uint64_t readsize[2] = {gndx, gndy / nproc};
    uint64_t offset[2] = {0LL, rank * readsize[1]};
    if (rank == nproc - 1)
    {
        // last process should read all the rest of columns
        readsize[1] = gndy - readsize[1] * (nproc - 1);
    }

    std::cout << "rank " << rank << " reads " << readsize[1]
              << " columns from offset " << offset[1] << std::endl;

    ADIOS_VARINFO *vT = adios_inq_var(f, "T");

    double *T = new double[vT->nsteps * readsize[0] * readsize[1]];

    // Create a 2D selection for the subset
    ADIOS_SELECTION *sel = adios_selection_boundingbox(2, offset, readsize);

    // Arrays are read by scheduling one or more of them
    // and performing the reads at once
    adios_schedule_read(f, sel, "T", 0, vT->nsteps, T);
    adios_perform_reads(f, 1);

    printData(T, readsize, offset, rank, vT->nsteps);
    adios_read_close(f);
    adios_free_varinfo(vT);
    delete[] T;
    // Terminate
    adios_selection_delete(sel);
    adios_read_finalize_method(ADIOS_READ_METHOD_BP);
    MPI_Finalize();
    return 0;
}
