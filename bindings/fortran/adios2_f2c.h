/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f.h : C-header glue code for functions called from Fortran modules
 *
 *  Created on: Aug 15, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_FORTRAN_ADIOS2_F2C_H_
#define BINDINGS_FORTRAN_ADIOS2_F2C_H_

#include <stddef.h>

#ifdef ADIOS2_HAVE_MPI_F
#include <mpi.h>
#endif

#include <adios2_c.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ADIOS2_HAVE_MPI_F
void adios2_init_f2c_(adios2_ADIOS **adios, MPI_Fint *comm,
                      const int *debug_mode, int *ierr);

void adios2_init_config_f2c_(adios2_ADIOS **adios, const char *config_file,
                             MPI_Fint *comm, const int *debug_mode, int *ierr);
#else
void adios2_init_f2c_(adios2_ADIOS **adios, const int *debug_mode, int *ierr);

void adios2_init_config_f2c_(adios2_ADIOS **adios, const char *config_file,
                             const int *debug_mode, int *ierr);
#endif

void adios2_declare_io_f2c_(adios2_IO **io, adios2_ADIOS **adios,
                            const char *io_name, int *ierr);

void adios2_set_param_f2c_(adios2_IO **io, const char *key, const char *value,
                           int *ierr);

void adios2_add_transport_f2c_(int *transport_index, adios2_IO **io,
                               const char *transport_type, int *ierr);

void adios2_set_transport_param_f2c_(adios2_IO **io, const int *transport_index,
                                     const char *key, const char *value,
                                     int *ierr);

void adios2_define_variable_f2c_(adios2_Variable **variable, adios2_IO **io,
                                 const char *variable_name, const int *type,
                                 const int *ndims, const int *shape,
                                 const int *start, const int *count,
                                 const int *constant_dims, int *ierr);

void adios2_open_f2c_(adios2_Engine **engine, adios2_IO **io, const char *name,
                      const int *open_mode, int *ierr);

#ifdef ADIOS2_HAVE_MPI_F
void adios2_open_new_comm_f2c_(adios2_Engine **engine, adios2_IO **io,
                               const char *name, const int *open_mode,
                               MPI_Fint *comm, int *ierr);
#endif

void adios2_write_f2c_(adios2_Engine **engine, adios2_Variable **variable,
                       const void *values, int *ierr);

void adios2_advance_f2c_(adios2_Engine **engine, int *ierr);

void adios2_close_f2c_(adios2_Engine **engine, int *ierr);

void adios2_finalize_f2c_(adios2_ADIOS **adios, int *ierr);

#ifdef __cplusplus
}
#endif

#endif /* BINDINGS_FORTRAN_ADIOS2_F2C_H_ */
