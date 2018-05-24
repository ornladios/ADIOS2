/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPWriter.c : C bindings version of C++11 helloBPWriter.cpp
 *
 *  Created on: Aug 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include <mpi.h>
#include <stdlib.h> // malloc, free

#include <adios2_c.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // application input, data in heap
    const size_t Nx = 10;
    float *myFloats;
    myFloats = malloc(sizeof(float) * Nx);

    unsigned int i;
    for (i = 0; i < Nx; ++i)
    {
        myFloats[i] = i;
    }

    adios2_adios *adiosH = adios2_init(MPI_COMM_WORLD, adios2_debug_mode_on);
    adios2_io *ioH = adios2_declare_io(adiosH, "BPFile_N2N");

    // dims are allocated in stack
    size_t shape[1];
    shape[0] = (size_t)size * Nx;

    size_t start[1];
    start[0] = (size_t)rank * Nx;

    size_t count[1];
    count[0] = Nx;

    adios2_variable *variableH = adios2_define_variable(
        ioH, "bpFloats", adios2_type_float, 1, shape, start, count,
        adios2_constant_dims_true, myFloats);

    adios2_engine *engineH =
        adios2_open(ioH, "myVector_c.bp", adios2_mode_write);

    adios2_put(engineH, variableH, myFloats, adios2_mode_deferred);
    adios2_close(engineH);
    adios2_finalize(adiosH);

    free(myFloats);

    MPI_Finalize();

    return 0;
}
