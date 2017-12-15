/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_variable.h : exposes some members of the Variable handler
 *
 *  Created on: Nov 10, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_C_ADIOS2_ADIOS2_C_VARIABLE_H_
#define BINDINGS_C_ADIOS2_ADIOS2_C_VARIABLE_H_

#include "adios2_c_types.h"

#include <stddef.h> //size_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Retrieve variable name (read-only)
 * @param variable handler
 * @return name
 */
const char *adios2_variable_name(const adios2_Variable *variable,
                                 size_t *length);

/**
 * Retrieve variable type (read-only)
 * @param variable handler
 * @return type
 */
adios2_type adios2_variable_type(const adios2_Variable *variable);

/**
 * Check if dimensions are constant
 * @param variable
 * @return 0: false (dimensions are not constant), 1: true dimensions are
 * declared constant
 */
int adios2_variable_is_constant_dims(const adios2_Variable *variable);

/**
 * Retrieve current variable number of dimensions (read-only)
 * @param variable
 * @return
 */
size_t adios2_variable_ndims(const adios2_Variable *variable);

/**
 * Retrieve current variable shape (read-only)
 * @param variable handler
 * @return shape
 */
const size_t *adios2_variable_shape(const adios2_Variable *variable);

/**
 * Retrieve current variable start (read-only)
 * @param variable handler
 * @return
 */
const size_t *adios2_variable_start(const adios2_Variable *variable);

/**
 * Retrieve current variable count (read-only)
 * @param variable
 * @return type
 */
const size_t *adios2_variable_count(const adios2_Variable *variable);

size_t adios2_variable_available_steps_start(const adios2_Variable *variable);

size_t adios2_variable_available_steps_count(const adios2_Variable *variable);

/**
 * Set new dimensions: shape, start and count
 * @param variable
 * @param ndims
 * @param shape
 * @param start
 * @param count
 */
void adios2_set_dimensions(adios2_Variable *variable, const size_t ndims,
                           const size_t *shape, const size_t *start,
                           const size_t *count);

/**
 * Set a new shape dimension
 * @param variable
 * @param ndims
 * @param shape
 */
void adios2_set_shape(adios2_Variable *variable, const size_t ndims,
                      const size_t *shape);

/**
 * Set new start and count dimensions
 * @param variable
 * @param start
 * @param count
 */
void adios2_set_selection(adios2_Variable *variable, const size_t ndims,
                          const size_t *start, const size_t *count);

/**
 * Set new step selection using step_start and step_count
 * @param variable
 * @param step_start
 * @param step_count
 */
void adios2_set_step_selection(adios2_Variable *variable,
                               const size_t step_start,
                               const size_t step_count);

/**
 * Returns the minimum required allocation (in number of elements of a certain
 * type, not bytes)
 * for the current selection
 * @param variable
 * @return memory size to be allocated by a pointer/vector to read this
 */
size_t adios2_selection_size(const adios2_Variable *variable);

/**
 * Get current data pointer, types must match
 * @param variable
 */
void *adios2_get_data(const adios2_Variable *variable);

/**
 * Sets current data pointer, types must match
 * @param variable
 * @param data
 */
void adios2_set_data(adios2_Variable *variable, const void *data);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* BINDINGS_C_ADIOS2_ADIOS2_C_VARIABLE_H_ */
