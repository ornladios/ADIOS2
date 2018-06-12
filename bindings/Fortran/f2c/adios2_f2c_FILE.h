/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_FILE.h
 *
 *  Created on: Feb 28, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_FORTRAN_F2C_ADIOS2_F2C_FILE_H_
#define ADIOS2_BINDINGS_FORTRAN_F2C_ADIOS2_F2C_FILE_H_

#include "adios2/ADIOSConfig.h"
#include <FC.h>
#include <adios2_c.h>

#include <iostream> //std::cerr
#include <stdint.h> // int64_t

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ADIOS2_HAVE_MPI_F
void FC_GLOBAL(adios2_fopen_f2c,
               adios2_FOPEN_F2C)(adios2_FILE **fh, const char *name,
                                 const int *mode, MPI_Fint *comm, int *ierr);

void FC_GLOBAL(adios2_fopen_config_f2c, adios2_FOPEN_CONFIG_F2C)(
    adios2_FILE **fh, const char *name, const int *mode, int *comm,
    const char *config_file, const char *io_in_config_file, int *ierr);

#else

void FC_GLOBAL(adios2_fopen_f2c, adios2_FOPEN_F2C)(adios2_FILE **fh,
                                                   const char *name,
                                                   const int *mode, int *ierr);

void FC_GLOBAL(adios2_fopen_config_f2c, adios2_FOPEN_CONFIG_F2C)(
    adios2_FILE **fh, const char *name, const int *mode,
    const char *config_file, const char *io_in_config_file, int *ierr);
#endif

void FC_GLOBAL(adios2_fwrite_value_f2c,
               adios2_FWRITE_VALUE_F2C)(adios2_FILE **fh, const char *name,
                                        const int *type, const void *data,
                                        const int *end_step, int *ierr);

void FC_GLOBAL(adios2_fwrite_f2c,
               adios2_FWRITE_F2C)(adios2_FILE **fh, const char *name,
                                  const int *type, const void *data,
                                  const int *ndims, const int64_t *shape,
                                  const int64_t *start, const int64_t *count,
                                  const int *end_step, int *ierr);

void FC_GLOBAL(adios2_fread_value_f2c,
               adios2_FREAD_VALUE_F2C)(adios2_FILE **fh, const char *name,
                                       const int *type, void *data,
                                       const int *end_step, int *ierr);

void FC_GLOBAL(adios2_fread_value_step_f2c,
               adios2_FREAD_VALUE_step_F2C)(adios2_FILE **fh, const char *name,
                                            const int *type, void *data,
                                            const int64_t *step_selection_start,
                                            int *ierr);

void FC_GLOBAL(adios2_fread_f2c,
               adios2_FREAD_F2C)(adios2_FILE **fh, const char *name,
                                 const int *type, void *data, const int *ndims,
                                 const int64_t *selection_start,
                                 const int64_t *selection_count,
                                 const int *end_step, int *ierr);

void FC_GLOBAL(adios2_fread_steps_f2c, adios2_FREAD_STEPS_F2C)(
    adios2_FILE **fh, const char *name, const int *type, void *data,
    const int *ndims, const int64_t *selection_start,
    const int64_t *selection_count, const int64_t *step_selection_start,
    const int64_t *step_selection_count, int *ierr);

void FC_GLOBAL(adios2_fclose_f2c, adios2_FCLOSE_F2C)(adios2_FILE **fh,
                                                     int *ierr);

#ifdef __cplusplus
}
#endif

#endif /* ADIOS2_BINDINGS_FORTRAN_F2C_ADIOS2_F2C_FILE_H */
