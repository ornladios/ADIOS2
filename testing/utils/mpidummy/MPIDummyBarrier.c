#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("MPIDummy R%02d: Startup\n", rank);

    // Handle the special case where this is used in a unit test within the
    // ADIOS build and is running under the MPMD wrapper script and no comm
    // split is being used.
    const char *ADIOS2_MPMD_WRAPPER_NOSPLIT =
        getenv("ADIOS2_MPMD_WRAPPER_NOSPLIT");
    if (!ADIOS2_MPMD_WRAPPER_NOSPLIT ||
        strcmp(ADIOS2_MPMD_WRAPPER_NOSPLIT, "0") == 0)
    {
        MPI_Comm mpiDummyComm;
        printf("MPIDummy R%02d: Splitting\n", rank);
        MPI_Comm_split(MPI_COMM_WORLD, 9999, rank, &mpiDummyComm);

        printf("MPIDummy R%02d: Freeing split comm\n", rank);
        MPI_Comm_free(&mpiDummyComm);
    }

    printf("MPIDummy R%02d: Barrier\n", rank);
    MPI_Barrier(MPI_COMM_WORLD);

    printf("MPIDummy R%02d: Finalize\n", rank);
    MPI_Finalize();

    return 0;
}
