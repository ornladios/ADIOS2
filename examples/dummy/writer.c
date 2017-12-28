/**
 * writer.c
 */

#include "mpi.h"

#include "sst.h"

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dummy.h"

int main(int argc, char **argv)
{
    int Rank = 0, Size = 0;
    MPI_Comm Comm = MPI_COMM_WORLD;

    SstStream Output;
    SstMetadata Meta;
    SstData Data;
    struct _SstStats Stats;
    int c;
    size_t DataSize = 10240;
    size_t Iterations = 1;
    size_t TimeStep = 0;
    int VerboseFlag = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(Comm, &Rank);
    MPI_Comm_size(Comm, &Size);

    while (1) {
        struct option LongOptions[] = {
            /* These options set a flag. */
            {"verbose", no_argument, 0, 'v'},
            {"brief", no_argument, NULL, 'b'},
            {"data_size", required_argument, 0, 'd'},
            {"iterations", required_argument, 0, 'i'},
            {0, 0, 0, 0}};
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long(argc, argv, "vbd:i:", LongOptions, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
        case 0:
            /* If this option set a flag, do nothing else now. */
            if (LongOptions[option_index].flag != 0)
                break;
            printf("option %s", LongOptions[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            break;

        case 'd':
            if (sscanf(optarg, "%zd", &DataSize) != 1) {
                if (Rank == 0) {
                    fprintf(stderr, "Argument \"%s\" not understood for data "
                                    "size, using %zd\n",
                            optarg, DataSize);
                }
            }
            break;
        case 'i':
            if (sscanf(optarg, "%zd", &Iterations) != 1) {
                if (Rank == 0) {
                    fprintf(stderr, "Argument \"%s\" not understood for "
                                    "iterations, using %zd\n",
                            optarg, Iterations);
                }
            }
            break;
        case 'v':
            VerboseFlag = 1;
            break;
        case 'b':
            VerboseFlag = 0;
            break;
        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            abort();
        }
    }

    Output = SstWriterOpen("test", "", Comm);

    SstSetStatsSave(Output, &Stats);

    for (TimeStep = 0; TimeStep < Iterations; TimeStep++) {
        /* test framework calls for creating dummy data */
        Meta = CreateDummyMetadata(TimeStep, Rank, Size, DataSize);
        Data = CreateDummyData(TimeStep, Rank, Size, DataSize);
        if (!ValidateDummyData(TimeStep, Rank, Size, 0, Data, DataSize)) {
            printf("validate data not valid on timestep %zd\n", TimeStep);
        }

        /* provide metadata and data for timestep */
        SstProvideTimestep(Output, Meta, Data, TimeStep);
    }

    /* (cleanly) shutdown this stream */
    SstWriterClose(Output);

    if ((Rank == 0) && VerboseFlag) {
        printf("Writer side rank 0 stats, Open duration %g secs, Close "
               "duration %g secs, Valid duration %g secs, Bytes Transferred "
               "%zd\n",
               Stats.OpenTimeSecs, Stats.CloseTimeSecs, Stats.ValidTimeSecs,
               Stats.BytesTransferred);
    }
    MPI_Finalize();
    return 0;
}
