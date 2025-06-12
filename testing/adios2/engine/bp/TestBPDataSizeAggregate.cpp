/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include <adios2.h>
#include <gtest/gtest.h>
#include <mpi.h>
#include <numeric>
#include <thread>

using namespace adios2;
MPI_Comm mpiComm;

class DSATest : public ::testing::Test
{
public:
    DSATest() = default;
};

TEST_F(DSATest, TestWriteUnbalancedData)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("ReadIO");

    io.SetEngine("BPFile");
    adios2::Engine bpReader = io.Open("unbalanced_data.bp", adios2::Mode::ReadRandomAccess);

    /*
    $ bpls unbalanced_data.bp
      float    Normals       {75786, 3}
      int32_t  connectivity  {446712}
      float    coordinates   {75786, 3}
      float    value         {75786}
    */

    auto var_norms = io.InquireVariable<float>("Normals");
    EXPECT_TRUE(var_norms);
    ASSERT_EQ(var_norms.ShapeID(), adios2::ShapeID::GlobalArray);
    ASSERT_EQ(var_norms.Steps(), 1);
    ASSERT_EQ(var_norms.Shape().size(), 2);

    auto var_conn = io.InquireVariable<int32_t>("connectivity");
    EXPECT_TRUE(var_conn);
    ASSERT_EQ(var_conn.ShapeID(), adios2::ShapeID::GlobalArray);
    ASSERT_EQ(var_conn.Steps(), 1);
    ASSERT_EQ(var_conn.Shape().size(), 1);

    auto var_coords = io.InquireVariable<float>("coordinates");
    EXPECT_TRUE(var_coords);
    ASSERT_EQ(var_coords.ShapeID(), adios2::ShapeID::GlobalArray);
    ASSERT_EQ(var_coords.Steps(), 1);
    ASSERT_EQ(var_coords.Shape().size(), 2);

    auto var_val = io.InquireVariable<float>("value");
    EXPECT_TRUE(var_val);
    ASSERT_EQ(var_val.ShapeID(), adios2::ShapeID::GlobalArray);
    ASSERT_EQ(var_val.Steps(), 1);
    ASSERT_EQ(var_val.Shape().size(), 1);

    std::vector<float> normals;
    std::vector<int32_t> conn;
    std::vector<float> coords;
    std::vector<float> value;

    bpReader.Get(var_norms, normals, adios2::Mode::Sync);
    ASSERT_EQ(normals.size(), var_norms.Shape()[0] * var_norms.Shape()[1]);

    bpReader.Get(var_conn, conn, adios2::Mode::Sync);
    ASSERT_EQ(conn.size(), var_conn.Shape()[0]);

    bpReader.Get(var_coords, coords, adios2::Mode::Sync);
    ASSERT_EQ(coords.size(), var_coords.Shape()[0] * var_coords.Shape()[1]);

    bpReader.Get(var_val, value, adios2::Mode::Sync);
    ASSERT_EQ(value.size(), var_val.Shape()[0]);

    bpReader.Close();

    // Now attempt to write the data back out using data-size based aggregation
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int worldRank, worldSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

    MPI_Finalize();
    return result;
}
