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

#include <stddef.h> //std::size_t

#ifdef __cplusplus
extern "C" {
#endif

typedef struct adios2_ADIOS adios2_ADIOS;
typedef struct adios2_IO adios2_IO;
typedef struct adios2_Variable adios2_Variable;
typedef struct adios2_Attribute adios2_Attribute;
typedef struct adios2_Engine adios2_Engine;

typedef enum {
    adios2_debug_mode_on = 0,
    adios2_debug_mode_off = 1,
} adios2_debug_mode;

typedef enum {
    adios2_constant_dims_true = 0,
    adios2_constant_dims_false = 1,
} adios2_constant_dims;

typedef enum {
    adios2_type_unknown = -1,

    adios2_type_char = 0,
    adios2_type_int = 1,
    adios2_type_float = 2,
    adios2_type_double = 3,
    adios2_type_float_complex = 4,
    adios2_type_double_complex = 5,

    adios2_type_int8_t = 6,
    adios2_type_int16_t = 7,
    adios2_type_int32_t = 8,
    adios2_type_int64_t = 9,

    adios2_type_string = 10,
    adios2_type_string_array = 11,

    adios2_type_signed_char,

    adios2_type_short,

    adios2_type_long_int,
    adios2_type_long_long_int,

    adios2_type_unsigned_char,
    adios2_type_unsigned_short,
    adios2_type_unsigned_int,
    adios2_type_unsigned_long_int,
    adios2_type_unsigned_long_long_int,

    adios2_type_uint8_t,
    adios2_type_uint16_t,
    adios2_type_uint32_t,
    adios2_type_uint64_t
} adios2_type;

typedef enum {
    adios2_mode_undefined = 0,
    adios2_mode_write = 1,
    adios2_mode_read = 2,
    adios2_mode_append = 3,
} adios2_mode;

typedef enum {
    adios2_step_mode_append = 0,
    adios2_step_mode_update = 1,
    adios2_step_mode_next_available = 2,
    adios2_step_mode_latest_available = 3
} adios2_step_mode;

typedef enum {
    adios2_step_status_other_error = -1,
    adios2_step_status_ok = 0,
    adios2_step_status_not_ready = 1,
    adios2_step_status_end_of_stream = 2
} adios2_step_status;

static size_t adios2_string_array_element_max_size = 4096;

#ifdef __cplusplus
} // end extern C
#endif

#endif /* ADIOS2_BINDINGS_C_ADIOS2_C_TYPES_H_ */
