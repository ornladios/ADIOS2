/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef TESTING_ADIOS2_BINDINGS_C_SMALLTESTDATA_C_H_
#define TESTING_ADIOS2_BINDINGS_C_SMALLTESTDATA_C_H_

#include <stdint.h>

const char dataStr[] = "A string variable";

size_t data_Nx = 10;

int8_t data_I8[10] = {0, 1, -2, 3, -4, 5, -6, 7, -8, 9};
int16_t data_I16[10] = {512, 513, -510, 515, -508, 517, -506, 519, -504, 521};
int32_t data_I32[10] = {131072, 131073,  -131070, 131075,  -131068,
                        131077, -131066, 131079,  -131064, 131081};
int64_t data_I64[10] = {8589934592,  8589934593, -8589934590, 8589934595,
                        -8589934588, 8589934597, -8589934586, 8589934599,
                        -8589934584, 8589934601};

uint8_t data_U8[10] = {128, 129, 130, 131, 132, 133, 134, 135, 136, 137};
uint16_t data_U16[10] = {32768, 32769, 32770, 32771, 32772,
                         32773, 32774, 32775, 32776, 32777};
uint32_t data_U32[10] = {2147483648, 2147483649, 2147483650, 2147483651,
                         2147483652, 2147483653, 2147483654, 2147483655,
                         2147483656, 2147483657};
uint64_t data_U64[10] = {9223372036854775808UL, 9223372036854775809UL,
                         9223372036854775810UL, 9223372036854775811UL,
                         9223372036854775812UL, 9223372036854775813UL,
                         9223372036854775814UL, 9223372036854775815UL,
                         9223372036854775816UL, 9223372036854775817UL};

float data_R32[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
double data_R64[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

#endif /* TESTING_ADIOS2_BINDINGS_C_SMALLTESTDATA_C_H_ */
