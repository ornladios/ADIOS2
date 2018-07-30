/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_operator.h :
 *
 *  Created on: Jul 30, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_C_C_ADIOS2_C_OPERATOR_H_
#define ADIOS2_BINDINGS_C_C_ADIOS2_C_OPERATOR_H_

#include "adios2_c_types.h"

#include "adios2/ADIOSMPICommOnly.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *adios2_operator_type(const adios2_operator *op, size_t *length);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* ADIOS2_BINDINGS_C_C_ADIOS2_C_OPERATOR_H_ */
