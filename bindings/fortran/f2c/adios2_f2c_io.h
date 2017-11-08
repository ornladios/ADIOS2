/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_io.h
 *
 *  Created on: Nov 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_FORTRAN_F2C_ADIOS2_F2C_IO_H_
#define BINDINGS_FORTRAN_F2C_ADIOS2_F2C_IO_H_

#include "adios2/ADIOSConfig.h"
#include <FC.h>
#include <adios2_c.h>

#ifdef __cplusplus
extern "C" {
#endif

void FC_GLOBAL(adios2_set_parameter_f2c,
               ADIOS2_SET_PARAMETER_F2C)(adios2_IO **io, const char *key,
                                         const char *value, int *ierr);

void FC_GLOBAL(adios2_add_transport_f2c,
               ADIOS2_ADD_TRANSPORT_F2C)(int *transport_index, adios2_IO **io,
                                         const char *transport_type, int *ierr);

void FC_GLOBAL(adios2_set_transport_parameter_f2c,
               ADIOS2_SET_TRANSPORT_PARAMETER_F2C)(adios2_IO **io,
                                                   const int *transport_index,
                                                   const char *key,
                                                   const char *value,
                                                   int *ierr);

void FC_GLOBAL(adios2_define_variable_f2c, ADIOS2_DEFINE_VARIABLE_F2C)(
    adios2_Variable **variable, adios2_IO **io, const char *variable_name,
    const int *type, const int *ndims, const int *shape, const int *start,
    const int *count, const int *constant_dims, int *ierr);

void FC_GLOBAL(adios2_open_f2c,
               ADIOS2_OPEN_F2C)(adios2_Engine **engine, adios2_IO **io,
                                const char *name, const int *open_mode,
                                int *ierr);

#ifdef ADIOS2_HAVE_MPI_F
void FC_GLOBAL(adios2_open_new_comm_f2c,
               ADIOS2_OPEN_NEW_COMM_F2C)(adios2_Engine **engine, adios2_IO **io,
                                         const char *name, const int *open_mode,
                                         MPI_Fint *comm, int *ierr);
#endif

#ifdef __cplusplus
}
#endif

#endif /* BINDINGS_FORTRAN_F2C_ADIOS2_F2C_IO_H_ */
