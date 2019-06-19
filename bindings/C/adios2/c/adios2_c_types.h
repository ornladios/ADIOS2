/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_types.h : basic adios2 types
 *
 *  Created on: Aug 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_C_ADIOS2_C_TYPES_H_
#define ADIOS2_BINDINGS_C_ADIOS2_C_TYPES_H_

#include <limits.h> // ULLONG_MAX
#include <stddef.h> // size_t
#include <stdint.h> // uint64_t

#include "adios2/common/ADIOSConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct adios2_adios adios2_adios;
typedef struct adios2_io adios2_io;
typedef struct adios2_variable adios2_variable;
typedef struct adios2_attribute adios2_attribute;
typedef struct adios2_engine adios2_engine;
typedef struct adios2_operator adios2_operator;

/**
 * @brief adios2_error return types for all ADIOS2 C API functions
 * Based on the library C++ standardized exceptions
 * https://en.cppreference.com/w/cpp/error/exception
 * Each error will issue a more detailed description in the standard error
 * output, stderr
 */
typedef enum
{
    /** success */
    adios2_error_none = 0,

    /**
     * user input error, on when adios2_debug_mode_on is passed to adios2_init
     * or adios2_init_config
     */
    adios2_error_invalid_argument = 1,

    /** low-level system error, e.g. system IO error */
    adios2_error_system_error = 2,

    /** runtime errors other than system errors, e.g. memory overflow */
    adios2_error_runtime_error = 3,

    /** any other error exception */
    adios2_error_exception = 4

} adios2_error;

typedef enum
{
    adios2_false = 0,
    adios2_true = 1,
} adios2_bool;

typedef enum
{
    adios2_debug_mode_off = 0,
    adios2_debug_mode_on = 1,
} adios2_debug_mode;

typedef enum
{
    adios2_constant_dims_false = 0,
    adios2_constant_dims_true = 1,
} adios2_constant_dims;

typedef enum
{
    adios2_advance_step_false = 0,
    adios2_advance_step_true = 1,
} adios2_advance_step;

typedef enum
{
    adios2_type_unknown = -1,

    adios2_type_string = 0,
    adios2_type_float = 1,
    adios2_type_double = 2,
    adios2_type_float_complex = 3,
    adios2_type_double_complex = 4,

    adios2_type_int8_t = 5,
    adios2_type_int16_t = 6,
    adios2_type_int32_t = 7,
    adios2_type_int64_t = 8,

    adios2_type_uint8_t = 9,
    adios2_type_uint16_t = 10,
    adios2_type_uint32_t = 11,
    adios2_type_uint64_t = 12,
} adios2_type;

typedef enum
{
    adios2_mode_undefined = 0,
    adios2_mode_write = 1,
    adios2_mode_read = 2,
    adios2_mode_append = 3,

    adios2_mode_deferred = 4,
    adios2_mode_sync = 5
} adios2_mode;

typedef enum
{
    adios2_step_mode_append = 0,
    adios2_step_mode_update = 1,
    adios2_step_mode_read = 2,
} adios2_step_mode;

typedef enum
{
    adios2_step_status_other_error = -1,
    adios2_step_status_ok = 0,
    adios2_step_status_not_ready = 1,
    adios2_step_status_end_of_stream = 2
} adios2_step_status;

typedef enum
{
    adios2_shapeid_unknown = -1,
    adios2_shapeid_global_value = 0,
    adios2_shapeid_global_array = 1,
    adios2_shapeid_joined_array = 2,
    adios2_shapeid_local_value = 3,
    adios2_shapeid_local_array = 4
} adios2_shapeid;

static size_t adios2_string_array_element_max_size = 4096;

static const uint64_t adios2_local_value_dim = ULLONG_MAX - 2;

#ifdef __cplusplus
} // end extern C
#endif

#endif /* ADIOS2_BINDINGS_C_ADIOS2_C_TYPES_H_ */
