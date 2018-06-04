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
 * Retrieve attribute name (read-only)
 * @param attribute handler
 * @param size name length
 * @return attribute name
 */
const char *adios2_attribute_name(const adios2_attribute *attribute,
                                  size_t *size);

/**
 * Retrieve attribute type (read-only)
 * @param attribute handler
 * @param size type length
 * @return attribute type
 */
const char *adios2_attribute_type(const adios2_attribute *attribute,
                                  size_t *size);

/**
 * Retrieve attribute data pointer (read-only)
 * @param attribute handler
 * @param size data size
 * @return data pointer
 */
const void *adios2_attribute_data(const adios2_attribute *attribute,
                                  size_t *size);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* ADIOS2_BINDINGS_C_C_ADIOS2_C_ATTRIBUTE_H_ */
