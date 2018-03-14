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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void FC_GLOBAL(adios2_set_parameter_f2c,
               ADIOS2_SET_PARAMETER_F2C)(adios2_io **io, const char *key,
                                         const char *value, int *ierr);

void FC_GLOBAL(adios2_add_transport_f2c,
               ADIOS2_ADD_TRANSPORT_F2C)(int *transport_index, adios2_io **io,
                                         const char *transport_type, int *ierr);

void FC_GLOBAL(adios2_set_transport_parameter_f2c,
               ADIOS2_SET_TRANSPORT_PARAMETER_F2C)(adios2_io **io,
                                                   const int *transport_index,
                                                   const char *key,
                                                   const char *value,
                                                   int *ierr);

void FC_GLOBAL(adios2_define_global_variable_f2c,
               ADIOS2_DEFINE_GLOBAL_VARIABLE_F2C)(adios2_variable **variable,
                                                  adios2_io **io,
                                                  const char *name,
                                                  const int *type, void *data,
                                                  int *ierr);

void FC_GLOBAL(adios2_define_variable_f2c, ADIOS2_DEFINE_VARIABLE_F2C)(
    adios2_variable **variable, adios2_io **io, const char *name,
    const int *type, const int *ndims, const int64_t *shape,
    const int64_t *start, const int64_t *count, const int *constant_dims,
    void *data, int *ierr);

void FC_GLOBAL(adios2_inquire_variable_f2c,
               ADIOS2_INQUIRE_VARIABLE_F2C)(adios2_variable **variable,
                                            adios2_io **io,
                                            const char *variable_name,
                                            int *ierr);

void FC_GLOBAL(adios2_remove_variable_f2c,
               ADIOS2_REMOVE_VARIABLE_F2C)(adios2_io **io,
                                           const char *variable_name,
                                           int *ierr);

void FC_GLOBAL(adios2_remove_all_variables_f2c,
               ADIOS2_REMOVE_ALL_VARIABLES_F2C)(adios2_io **io, int *ierr);

void FC_GLOBAL(adios2_define_attribute_f2c,
               ADIOS2_DEFINE_ATTRIBUTE_F2C)(adios2_attribute **attribute,
                                            adios2_io **io, const char *name,
                                            const int *type, const void *data,
                                            const int *elements, int *ierr);

void FC_GLOBAL(adios2_remove_attribute_f2c,
               ADIOS2_REMOVE_ATTRIBUTE_F2C)(adios2_io **io,
                                            const char *variable_name,
                                            int *ierr);

void FC_GLOBAL(adios2_remove_all_attributes_f2c,
               ADIOS2_REMOVE_ALL_ATTRIBUTES_F2C)(adios2_io **io, int *ierr);

#ifdef ADIOS2_HAVE_MPI_F
void FC_GLOBAL(adios2_open_new_comm_f2c,
               ADIOS2_OPEN_NEW_COMM_F2C)(adios2_engine **engine, adios2_io **io,
                                         const char *name, const int *open_mode,
                                         MPI_Fint *comm, int *ierr);
#endif

void FC_GLOBAL(adios2_open_f2c,
               ADIOS2_OPEN_F2C)(adios2_engine **engine, adios2_io **io,
                                const char *name, const int *open_mode,
                                int *ierr);

#ifdef __cplusplus
}
#endif

#endif /* BINDINGS_FORTRAN_F2C_ADIOS2_F2C_IO_H_ */
