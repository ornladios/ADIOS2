/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_attribute.h :
 *
 *  Created on: Jun 11, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_C_C_ADIOS2_C_ATTRIBUTE_H_
#define ADIOS2_BINDINGS_C_C_ADIOS2_C_ATTRIBUTE_H_

#include "adios2_c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Retrieve attribute name
 * @param name output name, must be pre-allocated
 * @param size name size
 * @param attribute handler
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_attribute_name(char *name, size_t *size,
                                   const adios2_attribute *attribute);

/**
 * Retrieve attribute type
 * @param type
 * @param attribute handler
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_attribute_type(adios2_type *type,
                                   const adios2_attribute *attribute);

/**
 * Retrieve attribute data pointer (read-only)
 * @param data output attribute values, must be pre-allocated
 * @param size data size
 * @param attribute handler
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_attribute_data(void *data, size_t *size,
                                   const adios2_attribute *attribute);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* ADIOS2_BINDINGS_C_C_ADIOS2_C_ATTRIBUTE_H_ */
