/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Created by Dmitry Ganyushin ganyushindi@ornl.gov
 */
#include "decomp.h"
#include "mpivars.h"
#include <adios2_c.h>
#include <stdio.h>
#include <stdlib.h>

void writer(adios2_adios *adios)
{
    int step, i;
    float *g;
    const int mincount = 2;
    const int maxcount = 5;
    const int numsteps = 5;
    adios2_step_status err;

    long long int fixed_shape = 0, fixed_start = 0, fixed_count = 0;

    /* Application variables
     g = 1D distributed array,
     global shape and per-process size is fixed */
    fixed_count = get_random(mincount, maxcount);
    g = malloc((size_t)fixed_count * sizeof(float));
    gather_decomp_1d(&fixed_count, &fixed_shape, &fixed_start);

    adios2_io *io = adios2_declare_io(adios, "output");
    size_t shape[1];
    shape[0] = (size_t)fixed_shape;

    size_t start[1];
    start[0] = (size_t)fixed_start;

    size_t count[1];
    count[0] = (size_t)fixed_count;

    adios2_variable *var_g = adios2_define_variable(io, "GlobalArray", adios2_type_float, 1, shape,
                                                    start, count, adios2_constant_dims_true);

    adios2_engine *engine = adios2_open(io, "adios2-global-array-1d-c.bp", adios2_mode_write);
    printf("Decmp rank = %d global shape = %lld local count = %lld  offset = "
           "%lld\n",
           rank, fixed_shape, fixed_count, fixed_start);
    for (step = 0; step < numsteps; step++)
    {
        for (i = 0; i < fixed_count; i++)
        {
            g[i] = (float)(rank + (step + 1) / 100.0);
        }

        adios2_begin_step(engine, adios2_step_mode_append, 10.0f, &err);
        adios2_put(engine, var_g, g, adios2_mode_deferred);
        adios2_end_step(engine);
    }
    // Close the output
    adios2_close(engine);
    free(g);

    if (rank == 0)
    {
        printf("Try the following: \n");
        printf("  bpls -la adios2-global-array-1d-c.bp GlobalArray -d -n "
               "%lld \n",
               fixed_shape);
        printf("  bpls -la adios2-global-array-1d-c.bp GlobalArray -d -t -n "
               "%lld \n ",
               fixed_shape);
        printf("  mpirun -n 2 ./adios2-global-array-1d-read-c \n");
    }
}

int main(int argc, char *argv[])
{
#if ADIOS2_USE_MPI
    init_mpi(123, argc, argv);
#endif

    {
#if ADIOS2_USE_MPI

        adios2_adios *adios = adios2_init_mpi(MPI_COMM_WORLD);
#else
        adios2_adios *adios = adios2_init();
#endif

        writer(adios);
        adios2_finalize(adios);
    }

#if ADIOS2_USE_MPI
    finalize_mpi();
#endif

    return 0;
}
