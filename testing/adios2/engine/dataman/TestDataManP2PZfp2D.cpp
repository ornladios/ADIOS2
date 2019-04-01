/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestDataManP2PZfp2D.cpp
 *
 *  Created on: Aug 16, 2018
 *      Author: Jason Wang
 */

#include "TestDataMan.h"

class DataManEngineTest : public ::testing::Test
{
public:
    DataManEngineTest() = default;
};

#ifdef ADIOS2_HAVE_ZEROMQ
#ifdef ADIOS2_HAVE_ZFP
TEST_F(DataManEngineTest, WriteRead_2D_P2P_Zfp)
{
    // set parameters
    Dims shape = {5, 5};
    Dims start = {0, 0};
    Dims count = {5, 5};
    size_t steps = 200;
    adios2::Params engineParams = {{"WorkflowMode", "stream"}};
    std::vector<adios2::Params> transportParams = {
        {{"Library", "ZMQ"},
         {"IPAddress", "127.0.0.1"},
         {"Port", "12307"},
         {"CompressionMethod", "zfp"},
         {"zfp:precision", "0.001"}}};

    // run workflow
    auto r = std::thread(DataManReaderP2P, shape, start, count, steps,
                         engineParams, transportParams);
    std::cout << "Reader thread started" << std::endl;
    auto w = std::thread(DataManWriter, shape, start, count, steps,
                         engineParams, transportParams);
    std::cout << "Writer thread started" << std::endl;
    w.join();
    std::cout << "Writer thread ended" << std::endl;
    r.join();
    std::cout << "Reader thread ended" << std::endl;
}
#endif // ZFP
#endif // ZEROMQ

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    int mpi_provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mpi_provided);
    std::cout << "MPI_Init_thread required Mode " << MPI_THREAD_MULTIPLE
              << " and provided Mode " << mpi_provided << std::endl;
    if (mpi_provided != MPI_THREAD_MULTIPLE)
    {
        MPI_Finalize();
        return 0;
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
