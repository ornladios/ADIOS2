/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_enums.h
 *
 *  Created on: Aug 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_C_ADIOS2_C_ENUMS_H_
#define ADIOS2_BINDINGS_C_ADIOS2_C_ENUMS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    adios2_debug_mode_on = 0,
    adios2_debug_mode_off = 1,
} adios2_debug_mode;

typedef enum {
    adios2_constant_dims_true = 0,
    adios2_constant_dims_false = 1,
} adios2_constant_dims;

typedef enum {

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

    adios2_type_string,
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
    adios2_open_mode_undefined,
    adios2_open_mode_write,
    adios2_open_mode_read,
    adios2_open_mode_append,
} adios2_open_mode;

#ifdef __cplusplus
} // end extern C
#endif

#endif /* ADIOS2_BINDINGS_C_ADIOS2_C_ENUMS_H_ */
