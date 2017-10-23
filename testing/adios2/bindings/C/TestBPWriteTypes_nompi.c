/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestBPWriteTypes.c
 *
 *  Created on: Aug 9, 2017
 *      Author: wgodoy
 */

#include <adios2_c.h>

#include "SmallTestData_c.h"

int main(int argc, char *argv[])
{
    adios2_ADIOS *adiosH = adios2_init_nompi(adios2_debug_mode_on);

    // IO
    adios2_IO *ioH = adios2_declare_io(adiosH, "CArrayTypes");
    // Set engine parameters
    adios2_set_engine(ioH, "BPFileWriter");
    adios2_set_parameter(ioH, "ProfileUnits", "Microseconds");
    adios2_set_parameter(ioH, "Threads", "1");

    // Set transport and parameters
    const unsigned int transportID = adios2_add_transport(ioH, "File");
    adios2_set_transport_param(ioH, transportID, "library", "fstream");

    // count dims are allocated in stack
    size_t count[1];
    count[0] = data_Nx;

    // Define variables in ioH
    {
        adios2_define_variable(ioH, "varI8", adios2_type_char, 1, NULL, NULL,
                               count, adios2_constant_dims_true);
        adios2_define_variable(ioH, "varI16", adios2_type_short, 1, NULL, NULL,
                               count, adios2_constant_dims_true);
        adios2_define_variable(ioH, "varI32", adios2_type_int, 1, NULL, NULL,
                               count, adios2_constant_dims_true);
        adios2_define_variable(ioH, "varI64", adios2_type_long_int, 1, NULL,
                               NULL, count, adios2_constant_dims_true);

        adios2_define_variable(ioH, "varU8", adios2_type_unsigned_char, 1, NULL,
                               NULL, count, adios2_constant_dims_true);
        adios2_define_variable(ioH, "varU16", adios2_type_unsigned_short, 1,
                               NULL, NULL, count, adios2_constant_dims_true);
        adios2_define_variable(ioH, "varU32", adios2_type_unsigned_int, 1, NULL,
                               NULL, count, adios2_constant_dims_true);
        adios2_define_variable(ioH, "varU64", adios2_type_unsigned_long_int, 1,
                               NULL, NULL, count, adios2_constant_dims_true);
        adios2_define_variable(ioH, "varR32", adios2_type_float, 1, NULL, NULL,
                               count, adios2_constant_dims_true);
        adios2_define_variable(ioH, "varR64", adios2_type_double, 1, NULL, NULL,
                               count, adios2_constant_dims_true);
    }
    // get variables
    adios2_Variable *varI8 = adios2_inquire_variable(ioH, "varI8");
    adios2_Variable *varI16 = adios2_inquire_variable(ioH, "varI16");
    adios2_Variable *varI32 = adios2_inquire_variable(ioH, "varI32");
    adios2_Variable *varI64 = adios2_inquire_variable(ioH, "varI64");
    adios2_Variable *varU8 = adios2_inquire_variable(ioH, "varU8");
    adios2_Variable *varU16 = adios2_inquire_variable(ioH, "varU16");
    adios2_Variable *varU32 = adios2_inquire_variable(ioH, "varU32");
    adios2_Variable *varU64 = adios2_inquire_variable(ioH, "varU64");
    adios2_Variable *varR32 = adios2_inquire_variable(ioH, "varR32");
    adios2_Variable *varR64 = adios2_inquire_variable(ioH, "varR64");

    // Open Engine handler, Write and Close
    adios2_Engine *engineH = adios2_open(ioH, "ctypes.bp", adios2_mode_write);

    adios2_put_sync(engineH, varI8, data_I8);
    adios2_put_sync(engineH, varI16, data_I16);
    adios2_put_sync(engineH, varI32, data_I32);
    adios2_put_sync(engineH, varI64, data_I64);

    adios2_put_sync(engineH, varU8, data_U8);
    adios2_put_sync(engineH, varU16, data_U16);
    adios2_put_sync(engineH, varU32, data_U32);
    adios2_put_sync(engineH, varU64, data_U64);

    adios2_put_sync(engineH, varR32, data_R32);
    adios2_put_sync(engineH, varR64, data_R64);

    adios2_close(engineH);

    // deallocate adiosH
    adios2_finalize(adiosH);
    return 0;
}
