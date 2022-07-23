/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPWriter.c : C bindings version of helloBPWriter.cpp
 *
 *  Created on: Aug 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include <stdio.h>  // printf
#include <stdlib.h> // malloc, free, exit

#include <adios2_c.h>

#if ADIOS2_USE_MPI
#include <mpi.h>
#endif

#include "scr.h"

void check_error(const int error)
{
    if (error)
    {
        printf("ERROR: %d\n", error);
        exit(error);
    }
}

void check_handler(const void *handler, const char *message)
{
    if (handler == NULL)
    {
        printf("ERROR: invalid %s handler \n", message);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    int rank, size;

#if ADIOS2_USE_MPI
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#else
    rank = 0;
    size = 1;
#endif

    // Collective over MPI_COMM_WORLD.
    // Rebuild any cached dataset, if possible.
    SCR_Init();

    adios2_error errio;
    // application input, data in heap
    const size_t Nx = 10;
    float *myFloats;
    myFloats = malloc(sizeof(float) * Nx);

    unsigned int i;
    for (i = 0; i < Nx; ++i)
    {
        myFloats[i] = (float)i;
    }

#if ADIOS2_USE_MPI
    adios2_adios *adios = adios2_init_mpi(MPI_COMM_WORLD);
#else
    adios2_adios *adios = adios2_init_serial();
#endif

    check_handler(adios, "adios");

    adios2_io *io = adios2_declare_io(adios, "BPFile_Write");
    check_handler(io, "io");

    // dims are allocated in the stack
    size_t shape[1];
    shape[0] = (size_t)size * Nx;

    size_t start[1];
    start[0] = (size_t)rank * Nx;

    size_t count[1];
    count[0] = Nx;

    adios2_variable *variable =
        adios2_define_variable(io, "bpFloats", adios2_type_float, 1, shape,
                               start, count, adios2_constant_dims_true);
    check_handler(variable, "variable");

    // Set this to 0 on any write error to indicate that the calling process failed to write.
    // Passed to SCR_Complete_output where an allreduce will check whether any process had an error.
    int scr_valid = 1;

    // Start a write phase (checkpoint and/or output).
    // Collective over MPI_COMM_WORLD
    // 
    // This holds processes in a barrier to ensure there is space if necessary.
    // This is useful in case an async flush has not yet finished.
    // It's also a simple way to avoid deleting files from a previous checkpoint
    // if a failure has happened on some node but has not yet been detected.
    //
    // The caller should provide a name for the dataset.
    // The name is returned during a restart, and it can be used to identify the checkpoint.
    // The name is also the way a user can request SCR to restart from a specific checkpoint.
    // It is common to encode timestamp info in this name.
    //
    // And the caller should specify dataset flags:
    //   SCR_FLAG_CHECKPOINT => dataset can be used to restart the application
    //   SCR_FLAG_OUTPUT => dataset must be written to parallel file system
    // These flags can be OR'd together:
    //   SCR_FLAG_CHECKPOINT | SCR_FLAG_OUTPUT
    SCR_Start_output("myVector", SCR_FLAG_CHECKPOINT);

    adios2_engine *engine = adios2_open(io, "myVector_c.bp", adios2_mode_write);
    check_handler(engine, "engine");

    errio = adios2_put(engine, variable, myFloats, adios2_mode_deferred);
    check_error(errio);

    errio = adios2_close(engine);
    check_error(errio);

    // Complete write phase.
    // Collective over MPI_COMM_WORLD.
    // Each process should pass 1 if it wrote its portion successfully and 0 if not.
    // The library executes an allreduce to determine whether all ranks succeeded.
    // If any rank failed, the dataset is considered to be invalid.
    // This is the point where the SCR library applies any redundancy schemes,
    // and where SCR initiate a flush to the parallel file system if needed.
    SCR_Complete_output(scr_valid);

    // deallocate adios
    errio = adios2_finalize(adios);
    check_error(errio);

    free(myFloats);

    // Flush any datasets from cache, if needed, or wait for those flushes to complete.
    // Collective over MPI_COMM_WORLD
    SCR_Finalize();

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return 0;
}
