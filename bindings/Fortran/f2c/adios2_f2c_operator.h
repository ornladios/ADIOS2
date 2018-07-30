/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_operator.h :
 *
 *  Created on: Jul 30, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_FORTRAN_F2C_ADIOS2_F2C_OPERATOR_H_
#define ADIOS2_BINDINGS_FORTRAN_F2C_ADIOS2_F2C_OPERATOR_H_

#include "adios2/ADIOSConfig.h"
#include <FC.h>
#include <adios2_c.h>

#include <stdint.h> // int64_t

#ifdef __cplusplus
extern "C" {
#endif

void FC_GLOBAL(adios2_operator_type_f2c,
               ADIOS2_OPERATOR_TYPE_F2C)(const adios2_operator **op,
                                         char name[1024], int *length,
                                         int *ierr);

#ifdef __cplusplus
}
#endif

#endif /* ADIOS2_BINDINGS_FORTRAN_F2C_ADIOS2_F2C_OPERATOR_H_ */
