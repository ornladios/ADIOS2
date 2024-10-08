/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestBPWriteStatsOnly.cpp : test the C bindings when only stats are written
 */

#include <adios2_c.h>
#include <gtest/gtest.h>
#include <vector>

class ADIOS2_C_API : public ::testing::Test
{
public:
    ADIOS2_C_API() { adiosH = adios2_init_serial(); }

    ~ADIOS2_C_API() { adios2_finalize(adiosH); }

    adios2_adios *adiosH;
};

TEST_F(ADIOS2_C_API, ADIOS2BPWriteStatsOnly)
{
    const char fname[] = "ADIOS2_C_API.ADIOS2BPWriteStatsOnly.bp";
    // write
    {
        adios2_io *ioH = adios2_declare_io(adiosH, "CWriteStats");
        adios2_set_engine(ioH, "BPFile");

        size_t shape[2];
        shape[0] = 5;
        shape[1] = 2;

        size_t start[2];
        start[0] = 0;
        start[1] = 0;

        size_t count[2];
        count[0] = 5;
        count[1] = 2;

        int32_t data_I32[10] = {131072, 131073,  -131070, 131075,  -131068,
                                131077, -131066, 131079,  -131064, 131081};
        adios2_define_variable(ioH, "varI32", adios2_type_int32_t, 2, shape, start, count,
                               adios2_constant_dims_true);
        adios2_variable *varI32 = adios2_inquire_variable(ioH, "varI32");
        adios2_store_stats_only(varI32, adios2_true);

        adios2_engine *engineH = adios2_open(ioH, fname, adios2_mode_write);
        adios2_put(engineH, varI32, data_I32, adios2_mode_deferred);
        adios2_close(engineH);
    }
    // read shape
    {
        adios2_io *ioH = adios2_declare_io(adiosH, "Reader");
        adios2_engine *engineH = adios2_open(ioH, fname, adios2_mode_readRandomAccess);
        size_t steps;
        adios2_steps(&steps, engineH);
        EXPECT_EQ(steps, 1);

        std::vector<int32_t> inI32(10);
        adios2_variable *varI32 = adios2_inquire_variable(ioH, "varI32");

        // test that the shape function returns the correct dimensions
        size_t cpu_shape[2];
        adios2_error e = adios2_variable_shape(cpu_shape, varI32);
        EXPECT_EQ(e, adios2_error_none);
        EXPECT_EQ(cpu_shape[0], 5);
        EXPECT_EQ(cpu_shape[1], 2);

        e = adios2_get(engineH, varI32, inI32.data(), adios2_mode_sync);
        EXPECT_EQ(e, adios2_error_runtime_error);
        adios2_close(engineH);
    }
}

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

    return result;
}
