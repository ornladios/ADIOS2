/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_variable.h : variable handle functions
 *
 *  Created on: Nov 12, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */
#ifndef BINDINGS_FORTRAN_F2C_ADIOS2_F2C_VARIABLE_H_
#define BINDINGS_FORTRAN_F2C_ADIOS2_F2C_VARIABLE_H_

#include "adios2/ADIOSConfig.h"
#include <FC.h>
#include <adios2_c.h>

#ifdef __cplusplus
extern "C" {
#endif

void FC_GLOBAL(adios2_variable_name_f2c,
               ADIOS2_VARIABLE_NAME_F2C)(const adios2_Variable **variable,
                                         char name[1024], int *length,
                                         int *ierr);

void FC_GLOBAL(adios2_variable_type_f2c,
               ADIOS2_VARIABLE_TYPE_F2C)(const adios2_Variable **variable,
                                         int *c_type, int *ierr);

void FC_GLOBAL(adios2_set_selection_f2c,
               ADIOS2_SET_SELECTION_F2C)(adios2_Variable **variable,
                                         const int *ndims, const int *start,
                                         const int *count, int *ierr);

#ifdef __cplusplus
}
#endif

#endif /* BINDINGS_FORTRAN_F2C_ADIOS2_F2C_VARIABLE_H_ */
