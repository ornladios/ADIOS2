/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPWriter_nompi.c : C bindings version of C++11 helloBPWriter_nompi.cpp
 *
 *  Created on: Aug 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <stdlib.h> //malloc, free

#include <adios2_c.h>

int main(int argc, char *argv[])
{
    // application input, data in heap
    const size_t Nx = 10;
    float *myFloats;
    myFloats = malloc(sizeof(float) * Nx);

    unsigned int i;
    for (i = 0; i < Nx; ++i)
    {
        myFloats[i] = (float)i;
    }

    adios2_ADIOS *adiosH = adios2_init_nompi(adios2_debug_mode_on);
    adios2_IO *ioH = adios2_declare_io(adiosH, "BPFile_N2N");

    // count dims are allocated in stack
    size_t count[1];
    count[0] = Nx;

    adios2_Variable *variableH = adios2_define_variable(
        ioH, "bpFloats", adios2_type_float, 1, NULL, NULL, count,
        adios2_constant_dims_true, myFloats);

    adios2_Engine *engineH =
        adios2_open(ioH, "myVector_c.bp", adios2_mode_write);

    adios2_put_sync(engineH, variableH, myFloats);

    adios2_close(engineH);
    adios2_finalize(adiosH);

    free(myFloats);

    return 0;
}
