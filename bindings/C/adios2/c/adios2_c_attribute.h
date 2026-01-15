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
 * For safe use, call this function first with NULL name parameter
 * to get the size, then preallocate the buffer (with room for '\0'
 * if desired), then call the function again with the buffer.
 * Then '\0' terminate it if desired.
 * @param name output, string without trailing '\0', NULL or preallocated buffer
 * @param size output, name size without '\0'
 * @param attribute handler
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_attribute_name(char *name, size_t *size, const adios2_attribute *attribute);

/**
 * Retrieve attribute type
 * @param type
 * @param attribute handler
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_attribute_type(adios2_type *type, const adios2_attribute *attribute);

/**
 * Retrieve attribute type in string form "char", "unsigned long", etc.
 * For safe use, call this function first with NULL name parameter
 * to get the size, then preallocate the buffer (with room for '\0'
 * if desired), then call the function again with the buffer.
 * Then '\0' terminate it if desired.
 * @param type output, string without trailing '\0', NULL or preallocated buffer
 * @param size output, type size without '\0'
 * @param attribute handler
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_attribute_type_string(char *type, size_t *size,
                                          const adios2_attribute *attribute);

/**
 * Checks if attribute is a single value or an array
 * @param result output, adios2_true: single value, adios2_false: array
 * @param attribute handler
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_attribute_is_value(adios2_bool *result, const adios2_attribute *attribute);

/**
 * Returns the number of elements (as in C++ STL size() function) if attribute
 * is a 1D array. If single value returns 1
 * @param size output, number of elements in attribute
 * @param attribute handler
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_attribute_size(size_t *size, const adios2_attribute *attribute);

/**
 * Retrieve attribute data pointer (read-only)
 * @param data output attribute values, must be pre-allocated
 * @param size data size
 * @param attribute handler
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_attribute_data(void *data, size_t *size, const adios2_attribute *attribute);

/**
 * Retrieve a single-value string attribute's data.
 *
 * This function supports a two-call pattern to safely retrieve string
 * attributes of unknown length:
 *   1. Call with data=NULL to get the string length
 *   2. Allocate a buffer of at least (length + 1) bytes
 *   3. Call again with the buffer to retrieve the string data
 *
 * The returned length does NOT include a null terminator. The caller
 * is responsible for null-terminating the string if needed.
 *
 * @param attribute handler for a single-value string attribute
 * @param data pre-allocated buffer for string data, or NULL to only query length
 * @param length output: the string length in bytes (excludes null terminator)
 * @return adios2_error 0: success, see enum adios2_error for errors
 *
 * Example usage:
 * @code
 *   size_t length;
 *   adios2_attribute_string_data(attr, NULL, &length);  // get length
 *   char *str = (char *)malloc(length + 1);
 *   adios2_attribute_string_data(attr, str, &length);   // get data
 *   str[length] = '\0';                                 // null terminate
 * @endcode
 */
adios2_error adios2_attribute_string_data(const adios2_attribute *attribute, char *data,
                                          size_t *length);

/**
 * Retrieve string data from a string array attribute.
 *
 * This function supports a two-call pattern to safely retrieve string
 * arrays of unknown lengths:
 *   1. Call adios2_attribute_size() to get the number of strings
 *   2. Allocate a lengths array of that size
 *   3. Call with data=NULL to get the length of each string
 *   4. Allocate each string buffer based on the lengths
 *   5. Call again with the allocated buffers to retrieve the string data
 *
 * The returned lengths do NOT include null terminators. The caller
 * is responsible for null-terminating each string if needed.
 *
 * @param attribute handler for a string array attribute
 * @param data array of pre-allocated buffers (char**), or NULL to only query lengths
 * @param lengths output: array of string lengths in bytes (excludes null terminators).
 *                Must be pre-allocated with size from adios2_attribute_size().
 * @return adios2_error 0: success, see enum adios2_error for errors
 *
 * Example usage:
 * @code
 *   size_t count;
 *   adios2_attribute_size(&count, attr);               // get array size
 *   size_t *lengths = (size_t *)malloc(count * sizeof(size_t));
 *   adios2_attribute_string_data_array(attr, NULL, lengths);  // get lengths
 *   char **strings = (char **)malloc(count * sizeof(char*));
 *   for (size_t i = 0; i < count; i++) {
 *       strings[i] = (char *)malloc(lengths[i] + 1);
 *   }
 *   adios2_attribute_string_data_array(attr, strings, lengths);  // get data
 *   for (size_t i = 0; i < count; i++) {
 *       strings[i][lengths[i]] = '\0';                // null terminate
 *   }
 * @endcode
 */
adios2_error adios2_attribute_string_data_array(const adios2_attribute *attribute, char **data,
                                                size_t *lengths);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* ADIOS2_BINDINGS_C_C_ADIOS2_C_ATTRIBUTE_H_ */
