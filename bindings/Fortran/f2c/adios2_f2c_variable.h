/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_variable.h : variable handle functions
 *
 *  Created on: Nov 12, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */
#ifndef ADIOS2_BINDINGS_FORTRAN_F2C_ADIOS2_F2C_VARIABLE_H_
#define ADIOS2_BINDINGS_FORTRAN_F2C_ADIOS2_F2C_VARIABLE_H_

#include "adios2/ADIOSConfig.h"
#include <FC.h>
#include <adios2_c.h>

#include <stdint.h> // int64_t

#ifdef __cplusplus
extern "C" {
#endif

void FC_GLOBAL(adios2_variable_name_f2c,
               ADIOS2_VARIABLE_NAME_F2C)(const adios2_variable **variable,
                                         char name[4096], int *length,
                                         int *ierr);

void FC_GLOBAL(adios2_variable_type_f2c,
               ADIOS2_VARIABLE_TYPE_F2C)(const adios2_variable **variable,
                                         int *c_type, int *ierr);

void FC_GLOBAL(adios2_variable_ndims_f2c,
               ADIOS2_VARIABLE_NDIMS_F2C)(const adios2_variable **variable,
                                          int *ndims, int *ierr);

void FC_GLOBAL(adios2_variable_shape_f2c,
               ADIOS2_VARIABLE_SHAPE_F2C)(const adios2_variable **variable,
                                          int64_t *shape, int *ierr);

void
    FC_GLOBAL(adios2_variable_steps_start_f2c,
              ADIOS2_VARIABLE_STEPS_START_F2C)(const adios2_variable **variable,
                                               int64_t *steps_start, int *ierr);

void FC_GLOBAL(adios2_variable_steps_f2c,
               ADIOS2_VARIABLE_STEPS_F2C)(const adios2_variable **variable,
                                          int64_t *steps_count, int *ierr);

void FC_GLOBAL(adios2_set_shape_f2c,
               ADIOS2_SET_SHAPE_F2C)(adios2_variable **variable,
                                     const int *ndims, const int64_t *shape,
                                     int *ierr);

void FC_GLOBAL(adios2_set_selection_f2c,
               ADIOS2_SET_SELECTION_F2C)(adios2_variable **variable,
                                         const int *ndims, const int64_t *start,
                                         const int64_t *count, int *ierr);

void FC_GLOBAL(adios2_set_step_selection_f2c,
               ADIOS2_SET_STEP_SELECTION_F2C)(adios2_variable **variable,
                                              const int64_t *step_start,
                                              const int64_t *step_count,
                                              int *ierr);

#ifdef __cplusplus
}
#endif

#endif /* ADIOS2_BINDINGS_FORTRAN_F2C_ADIOS2_F2C_VARIABLE_H_ */
