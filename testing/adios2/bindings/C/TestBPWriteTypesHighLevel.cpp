/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestBPWriteTypesHighLevel.cpp : C-bindings high-level stream API
 *
 *  Created on: Feb 28, 2018
 *      Author: William F Godoy
 */

#include <adios2_c.h>

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

#include <gtest/gtest.h>

#include "SmallTestData_c.h"

class BPWriteTypesHighLevel : public ::testing::Test
{
public:
    BPWriteTypesHighLevel() = default;
};

TEST_F(BPWriteTypesHighLevel, ADIOS2BPWriteTypes)
{
    int rank = 0;
    int size = 1;
#ifdef ADIOS2_HAVE_MPI

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    adios2_FILE *fhw =
        adios2_fopen("ctypes_hl.bp", adios2_mode_write, MPI_COMM_WORLD);
#else
    adios2_FILE *fhw = adios2_fopen_nompi("ctypes_hl.bp", adios2_mode_write);
#endif

    size_t shape[1];
    shape[0] = (size_t)size * data_Nx;

    size_t start[1];
    start[0] = (size_t)rank * data_Nx;

    size_t count[1];
    count[0] = data_Nx;

    adios2_fwrite(fhw, "varString", adios2_type_string, "hello stream", 1,
                  shape, start, count, 0);
    adios2_fwrite(fhw, "varI8", adios2_type_int8_t, data_I8, 1, shape, start,
                  count, 0);
    adios2_fwrite(fhw, "varI16", adios2_type_int16_t, data_I16, 1, shape, start,
                  count, 0);
    adios2_fwrite(fhw, "varI32", adios2_type_int32_t, data_I32, 1, shape, start,
                  count, 0);
    adios2_fwrite(fhw, "varI64", adios2_type_int64_t, data_I64, 1, shape, start,
                  count, 0);

    adios2_fwrite(fhw, "varU8", adios2_type_uint8_t, data_U8, 1, shape, start,
                  count, 0);
    adios2_fwrite(fhw, "varU16", adios2_type_uint16_t, data_U16, 1, shape,
                  start, count, 0);
    adios2_fwrite(fhw, "varU32", adios2_type_uint32_t, data_U32, 1, shape,
                  start, count, 0);
    adios2_fwrite(fhw, "varU64", adios2_type_uint64_t, data_U64, 1, shape,
                  start, count, 0);
    adios2_fwrite(fhw, "varR32", adios2_type_float, data_R32, 1, shape, start,
                  count, 0);
    adios2_fwrite(fhw, "varR64", adios2_type_double, data_R64, 1, shape, start,
                  count, 0);

    adios2_fclose(fhw);

    {
// for reading
#ifdef ADIOS2_HAVE_MPI
        adios2_FILE *fhr =
            adios2_fopen("ctypes_hl.bp", adios2_mode_read, MPI_COMM_WORLD);
#else
        adios2_FILE *fhr = adios2_fopen_nompi("ctypes_hl.bp", adios2_mode_read);
#endif
        const size_t data_size = 10;

        char *idata_String = (char *)malloc(13);
        int8_t *idata_I8 = (int8_t *)malloc(data_size * sizeof(int8_t));
        int16_t *idata_I16 = (int16_t *)malloc(data_size * sizeof(int16_t));
        int32_t *idata_I32 = (int32_t *)malloc(data_size * sizeof(int32_t));
        int64_t *idata_I64 = (int64_t *)malloc(data_size * sizeof(int64_t));
        uint8_t *idata_U8 = (uint8_t *)malloc(data_size * sizeof(uint8_t));
        uint16_t *idata_U16 = (uint16_t *)malloc(data_size * sizeof(uint16_t));
        uint32_t *idata_U32 = (uint32_t *)malloc(data_size * sizeof(uint32_t));
        uint64_t *idata_U64 = (uint64_t *)malloc(data_size * sizeof(uint64_t));
        float *idata_R32 = (float *)malloc(data_size * sizeof(float));
        double *idata_R64 = (double *)malloc(data_size * sizeof(double));

        size_t selection_start[1];
        selection_start[0] = 0;

        size_t selection_count[1];
        selection_count[0] = data_size;

        adios2_fread(fhr, "varString", adios2_type_string, idata_String, 1,
                     NULL, NULL, 0);

        adios2_fread(fhr, "varI8", adios2_type_int8_t, idata_I8, 1,
                     selection_start, selection_count, 0);
        adios2_fread(fhr, "varI16", adios2_type_int16_t, idata_I16, 1,
                     selection_start, selection_count, 0);
        adios2_fread(fhr, "varI32", adios2_type_int32_t, idata_I32, 1,
                     selection_start, selection_count, 0);
        adios2_fread(fhr, "varI64", adios2_type_int64_t, idata_I64, 1,
                     selection_start, selection_count, 0);
        adios2_fread(fhr, "varU8", adios2_type_uint8_t, idata_U8, 1,
                     selection_start, selection_count, 0);
        adios2_fread(fhr, "varU16", adios2_type_uint16_t, idata_U16, 1,
                     selection_start, selection_count, 0);
        adios2_fread(fhr, "varU32", adios2_type_uint32_t, idata_U32, 1,
                     selection_start, selection_count, 0);
        adios2_fread(fhr, "varU64", adios2_type_uint64_t, idata_U64, 1,
                     selection_start, selection_count, 0);

        adios2_fread(fhr, "varR32", adios2_type_float, idata_R32, 1,
                     selection_start, selection_count, 0);
        adios2_fread(fhr, "varR64", adios2_type_double, idata_R64, 1,
                     selection_start, selection_count, 0);

        EXPECT_EQ(strcmp(idata_String, "hello stream"), 0);

        for (size_t i = 0; i < data_size; ++i)
        {
            EXPECT_EQ(idata_I8[i], data_I8[i]);
            EXPECT_EQ(idata_I16[i], data_I16[i]);
            EXPECT_EQ(idata_I32[i], data_I32[i]);
            EXPECT_EQ(idata_I64[i], data_I64[i]);

            EXPECT_EQ(idata_U8[i], data_U8[i]);
            EXPECT_EQ(idata_U16[i], data_U16[i]);
            EXPECT_EQ(idata_U32[i], data_U32[i]);
            EXPECT_EQ(idata_U64[i], data_U64[i]);

            EXPECT_EQ(idata_R32[i], data_R32[i]);
            EXPECT_EQ(idata_R64[i], data_R64[i]);
        }

        free(idata_String);
        free(idata_I8);
        free(idata_I16);
        free(idata_I32);
        free(idata_I64);
        free(idata_U8);
        free(idata_U16);
        free(idata_U32);
        free(idata_U64);
    }
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    int result = -1;
    try
    {
        ::testing::InitGoogleTest(&argc, argv);
        result = RUN_ALL_TESTS();
    }
    catch (std::exception &e)
    {
        result = 1;
    }

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
