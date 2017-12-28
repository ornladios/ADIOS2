#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpi.h"

#include "sst.h"
#include "dummy.h"

int main(int argc, char **argv)
{
    int Rank, Size;
    SstFullMetadata Meta;
    void **completions = NULL;
    SstStream Input;
    struct _SstStats Stats;
    char **buffers;
    int VerboseFlag = 0;
    int ReadSet = 0;
    int TimeStep;

    MPI_Comm Comm = MPI_COMM_WORLD;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(Comm, &Rank);
    MPI_Comm_size(Comm, &Size);

    while (1) {
        struct option LongOptions[] = {
            /* These options set a flag. */
            {"verbose", no_argument, NULL, 'v'},
            {"data_size", required_argument, NULL, 'd'},
            {"read_set", required_argument, NULL, 'r'},
            {0, 0, 0, 0}};
        int c;
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long(argc, argv, "vbd:r:", LongOptions, &option_index);

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

        case 'r':
            //            if (sscanf(optarg, "%zd", &DataSize) != 1) {
            //                if (Rank == 0) {
            //                    fprintf(stderr, "Argument \"%s\" not
            //                    understood for data size, using %zd\n",
            //                    optarg, DataSize);
            //                }
            //            }
            break;
        case 'v':
            VerboseFlag = 1;
            break;
        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            abort();
        }
    }

    Input = SstReaderOpen("test", "", Comm);
    SstSetStatsSave(Input, &Stats);

    TimeStep = 0;
    while ((Meta = SstGetMetadata(Input, TimeStep))) {
        SstStatusValue Status;
        if (!completions) {
            completions =
                malloc(sizeof(completions[0]) * Meta->WriterCohortSize);
            memset(completions, 0,
                   sizeof(completions[0]) * Meta->WriterCohortSize);
            buffers = malloc(sizeof(buffers[0]) * Meta->WriterCohortSize);
            memset(buffers, 0, sizeof(buffers[0]) * Meta->WriterCohortSize);
        }

        if (ReadSet == 0 /* OddReadSet*/) {
            for (int i = Rank % 2; i < Meta->WriterCohortSize; i += 2) {
                /* only filling in every other one */
                buffers[i] = malloc(Meta->WriterMetadata[i]->DataSize);
                completions[i] = SstReadRemoteMemory(
                    Input, i /* Rank */, TimeStep, 3 /* offset */,
                    Meta->WriterMetadata[i]->DataSize - 3, buffers[i],
                    Meta->DP_TimestepInfo ? Meta->DP_TimestepInfo[i] : NULL);
            }
        } else if (ReadSet == 1 /* MxNReadSet */) {
            double MyStart = ((double)Rank) / Size,
                   MyEnd = ((double)(Rank + 1)) / Size;
            for (int i = 0; i < Meta->WriterCohortSize; i++) {
                double ThisStart = ((double)i) / Meta->WriterCohortSize,
                       ThisEnd = ((double)i + 1) / Meta->WriterCohortSize;
                if ((ThisStart <= MyEnd) && (ThisEnd <= MyStart)) {
                    printf("Rank %d, adding writer %d\n", Rank, i);
                    buffers[i] = malloc(Meta->WriterMetadata[i]->DataSize);
                    completions[i] = SstReadRemoteMemory(
                        Input, i /* Rank */, TimeStep, 3 /* offset */,
                        Meta->WriterMetadata[i]->DataSize - 3, buffers[i],
                        Meta->DP_TimestepInfo ? Meta->DP_TimestepInfo[i]
                                              : NULL);
                }
            }
        }
        for (int i = 0; i < Meta->WriterCohortSize; i++) {
            if (completions[i]) {
                SstWaitForCompletion(Input, completions[i]);
                if (ValidateDummyData(TimeStep, i, Meta->WriterCohortSize, 3,
                                      buffers[i],
                                      Meta->WriterMetadata[i]->DataSize) != 0) {
                    printf("Bad data from rank %d\n", i);
                }
            }
        }

        SstReleaseStep(Input, TimeStep);
        TimeStep++;
        Status = SstAdvanceStep(Input, TimeStep);
        if (Status != SstSuccess)
            break;
    }
    SstReaderClose(Input);

    if ((Rank == 0) && VerboseFlag) {
        double bandwidth = Stats.BytesTransferred / Stats.ValidTimeSecs;
        printf("Reader side rank 0 stats: Open duration %g secs, Close "
               "duration %g secs\n\t\tValid duration %g secs, Bytes "
               "Transferred %zd\n",
               Stats.OpenTimeSecs, Stats.CloseTimeSecs, Stats.ValidTimeSecs,
               Stats.BytesTransferred);
        printf("Bandwidth to this rank %g Mbits/sec\n",
               bandwidth / (1000.0 * 1000.0 * 8));
    }
    MPI_Finalize();
    return 0;
}
