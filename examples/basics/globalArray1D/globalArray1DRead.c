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

void reader(adios2_adios *adios)
{
    int step;
    float *g;
    const char *streamname = "adios2-global-array-1d-c.bp";
    adios2_step_status err;

    long long int fixed_shape = 0, fixed_start = 0, fixed_count = 0;

    adios2_io *io = adios2_declare_io(adios, "input");
    size_t shape[1];
    shape[0] = (size_t)fixed_shape;

    adios2_engine *engine = adios2_open(io, streamname, adios2_mode_read);
    step = 0;
    do
    {
        adios2_begin_step(engine, adios2_step_mode_read, 10.0, &err);
        adios2_variable *var_g = adios2_inquire_variable(io, "GlobalArray");
        if (step == 0)
        {
            /* fixed_shape is allocated in the next call*/
            adios2_variable_shape(shape, var_g);
            fixed_shape = (long long int)shape[0];
            decomp_1d(fixed_shape, &fixed_start, &fixed_count);
            g = malloc((size_t)fixed_count * sizeof(float));

            printf("Read plan rank = %d global shape = %lld local count = %lld "
                   "offset = %lld\n",
                   rank, fixed_shape, fixed_count, fixed_start);
        }
        adios2_end_step(engine);
        step++;
    } while (err != adios2_step_status_end_of_stream);
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
        reader(adios);
        adios2_finalize(adios);
    }

#if ADIOS2_USE_MPI
    finalize_mpi();
#endif

    return 0;
}
