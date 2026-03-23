/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * TestMultiStreamBiDir - Tests multiple simultaneous bidirectional SST streams.
 *
 * Mirrors a common HPC coupling pattern where a "server" process communicates
 * with two separate "client" process groups, each via a pair of SST streams
 * (one in each direction).  This exercises:
 *   - Multiple concurrent SST writer and reader engines per process
 *   - Shared RDMA fabric state with reference counting across streams
 *   - Clean shutdown of all streams
 *
 * Usage:  TestMultiStreamBiDir <engine> <role>
 *   role -1 = server  (writes s2c_0, s2c_1;  reads c2s_0, c2s_1)
 *   role  0 = client0  (reads s2c_0;  writes c2s_0)
 *   role  1 = client1  (reads s2c_1;  writes c2s_1)
 */

#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include <adios2.h>

#if ADIOS2_USE_MPI
#include <mpi.h>
#endif

static const int NSteps = 5;
static const std::size_t Nx = 100;

static int runServer(adios2::ADIOS &adios, const std::string &engine, int mpiRank, int mpiSize)
{
    adios2::IO wIO0 = adios.DeclareIO("WriteIO_0");
    adios2::IO wIO1 = adios.DeclareIO("WriteIO_1");
    adios2::IO rIO0 = adios.DeclareIO("ReadIO_0");
    adios2::IO rIO1 = adios.DeclareIO("ReadIO_1");

    wIO0.SetEngine(engine);
    wIO1.SetEngine(engine);
    rIO0.SetEngine(engine);
    rIO1.SetEngine(engine);

    auto wVar0 = wIO0.DefineVariable<double>("data", {Nx * mpiSize}, {Nx * mpiRank}, {Nx});
    auto wVar1 = wIO1.DefineVariable<double>("data", {Nx * mpiSize}, {Nx * mpiRank}, {Nx});

    // Open writers first (they create the .sst contact files)
    adios2::Engine writer0 = wIO0.Open("s2c_0", adios2::Mode::Write);
    adios2::Engine writer1 = wIO1.Open("s2c_1", adios2::Mode::Write);

    // Then open readers
    adios2::Engine reader0 = rIO0.Open("c2s_0", adios2::Mode::Read);
    adios2::Engine reader1 = rIO1.Open("c2s_1", adios2::Mode::Read);

    std::vector<double> sendBuf(Nx);
    std::vector<double> recvBuf0(Nx * mpiSize);
    std::vector<double> recvBuf1(Nx * mpiSize);

    for (int step = 0; step < NSteps; ++step)
    {
        std::iota(sendBuf.begin(), sendBuf.end(), step * 1000.0 + mpiRank * Nx);

        writer0.BeginStep();
        writer0.Put(wVar0, sendBuf.data());
        writer0.EndStep();

        writer1.BeginStep();
        writer1.Put(wVar1, sendBuf.data());
        writer1.EndStep();

        auto status0 = reader0.BeginStep();
        if (status0 != adios2::StepStatus::OK)
        {
            std::cerr << "Server: unexpected step status from client0" << std::endl;
            return 1;
        }
        auto rVar0 = rIO0.InquireVariable<double>("data");
        reader0.Get(rVar0, recvBuf0.data());
        reader0.EndStep();

        auto status1 = reader1.BeginStep();
        if (status1 != adios2::StepStatus::OK)
        {
            std::cerr << "Server: unexpected step status from client1" << std::endl;
            return 1;
        }
        auto rVar1 = rIO1.InquireVariable<double>("data");
        reader1.Get(rVar1, recvBuf1.data());
        reader1.EndStep();
    }

    writer0.Close();
    writer1.Close();
    reader0.Close();
    reader1.Close();

    if (mpiRank == 0)
        std::cout << "Server: done" << std::endl;
    return 0;
}

static int runClient(adios2::ADIOS &adios, const std::string &engine, int clientId, int mpiRank,
                     int mpiSize)
{
    std::string readStream = "s2c_" + std::to_string(clientId);
    std::string writeStream = "c2s_" + std::to_string(clientId);

    adios2::IO rIO = adios.DeclareIO("ReadIO");
    adios2::IO wIO = adios.DeclareIO("WriteIO");

    rIO.SetEngine(engine);
    wIO.SetEngine(engine);

    auto wVar = wIO.DefineVariable<double>("data", {Nx * mpiSize}, {Nx * mpiRank}, {Nx});

    // Client opens reader first (waits for server's .sst file), then writer
    adios2::Engine reader = rIO.Open(readStream, adios2::Mode::Read);
    adios2::Engine writer = wIO.Open(writeStream, adios2::Mode::Write);

    std::vector<double> sendBuf(Nx);
    std::vector<double> recvBuf(Nx * mpiSize);

    for (int step = 0; step < NSteps; ++step)
    {
        auto status = reader.BeginStep();
        if (status != adios2::StepStatus::OK)
        {
            std::cerr << "Client" << clientId << ": unexpected step status" << std::endl;
            return 1;
        }
        auto rVar = rIO.InquireVariable<double>("data");
        reader.Get(rVar, recvBuf.data());
        reader.EndStep();

        std::iota(sendBuf.begin(), sendBuf.end(), step * 2000.0 + clientId * 100.0 + mpiRank * Nx);
        writer.BeginStep();
        writer.Put(wVar, sendBuf.data());
        writer.EndStep();
    }

    reader.Close();
    writer.Close();

    if (mpiRank == 0)
        std::cout << "Client" << clientId << ": done" << std::endl;
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <engine> <role: -1|0|1>" << std::endl;
        return 1;
    }

    std::string engine = argv[1];
    int role = std::atoi(argv[2]);

    int mpiRank = 0, mpiSize = 1;

#if ADIOS2_USE_MPI
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    // Split MPI_COMM_WORLD by role so that each role group gets its own
    // communicator.  This allows the test to be launched either as three
    // separate MPI jobs or in a single MPMD srun invocation.
    MPI_Comm roleComm;
    int color = role + 2; // map {-1,0,1} -> {1,2,3} so every color is non-negative
    int key;
    MPI_Comm_rank(MPI_COMM_WORLD, &key);
    MPI_Comm_split(MPI_COMM_WORLD, color, key, &roleComm);

    MPI_Comm_rank(roleComm, &mpiRank);
    MPI_Comm_size(roleComm, &mpiSize);
#endif

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(roleComm);
#else
    adios2::ADIOS adios;
#endif

    int result = 0;
    if (role == -1)
    {
        result = runServer(adios, engine, mpiRank, mpiSize);
    }
    else if (role == 0 || role == 1)
    {
        result = runClient(adios, engine, role, mpiRank, mpiSize);
    }
    else
    {
        std::cerr << "Invalid role: " << role << std::endl;
        result = 1;
    }

#if ADIOS2_USE_MPI
    MPI_Comm_free(&roleComm);
    MPI_Finalize();
#endif

    return result;
}
