/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include <adios2.h>
#include <gtest/gtest.h>
#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif
#include <numeric>
#include <thread>

using namespace adios2;
int mpiRank = 0;
int mpiSize = 1;
int mpiGroup;
MPI_Comm mpiComm;
int print_lines = 0;

char runMode;

class SscEngineTest : public ::testing::Test
{
public:
    SscEngineTest() = default;
};

template <class T>
void PrintData(const T *data, const size_t step, const Dims &start,
               const Dims &count)
{
    size_t size = std::accumulate(count.begin(), count.end(), 1,
                                  std::multiplies<size_t>());
    std::cout << "Rank: " << mpiRank << " Step: " << step << " Size:" << size
              << "\n";
    size_t printsize = 128;

    if (size < printsize)
    {
        printsize = size;
    }
    int s = 0;
    for (size_t i = 0; i < printsize; ++i)
    {
        ++s;
        std::cout << data[i] << " ";
        if (s == count[1])
        {
            std::cout << std::endl;
            s = 0;
        }
    }

    std::cout << "]" << std::endl;
}

template <class T>
void GenDataRecursive(std::vector<size_t> start, std::vector<size_t> count,
                      std::vector<size_t> shape, size_t n0, size_t y,
                      std::vector<T> &vec)
{
    for (size_t i = 0; i < count[0]; i++)
    {
        size_t i0 = n0 * count[0] + i;
        size_t z = y * shape[0] + (i + start[0]);

        auto start_next = start;
        auto count_next = count;
        auto shape_next = shape;
        start_next.erase(start_next.begin());
        count_next.erase(count_next.begin());
        shape_next.erase(shape_next.begin());

        if (start_next.size() == 1)
        {
            for (size_t j = 0; j < count_next[0]; j++)
            {
                vec[i0 * count_next[0] + j] =
                    z * shape_next[0] + (j + start_next[0]);
            }
        }
        else
        {
            GenDataRecursive(start_next, count_next, shape_next, i0, z, vec);
        }
    }
}

template <class T>
void GenData(std::vector<T> &vec, const size_t step,
             const std::vector<size_t> &start, const std::vector<size_t> &count,
             const std::vector<size_t> &shape)
{
    size_t total_size = std::accumulate(count.begin(), count.end(), 1,
                                        std::multiplies<size_t>());
    vec.resize(total_size);
    GenDataRecursive(start, count, shape, 0, 0, vec);
}

template <class T>
void VerifyData(const std::complex<T> *data, size_t step, const Dims &start,
                const Dims &count, const Dims &shape)
{
    size_t size = std::accumulate(count.begin(), count.end(), 1,
                                  std::multiplies<size_t>());
    std::vector<std::complex<T>> tmpdata(size);
    GenData(tmpdata, step, start, count, shape);
    for (size_t i = 0; i < size; ++i)
    {
        ASSERT_EQ(data[i], tmpdata[i]);
    }
    if (print_lines < 32)
    {
        PrintData(data, step, start, count);
        ++print_lines;
    }
}

template <class T>
void VerifyData(const T *data, size_t step, const Dims &start,
                const Dims &count, const Dims &shape)
{
    size_t size = std::accumulate(count.begin(), count.end(), 1,
                                  std::multiplies<size_t>());
    bool compressed = false;
    std::vector<T> tmpdata(size);
    if (print_lines < 32)
    {
        PrintData(data, step, start, count);
        ++print_lines;
    }
    GenData(tmpdata, step, start, count, shape);
    for (size_t i = 0; i < size; ++i)
    {
        if (!compressed)
        {
            ASSERT_EQ(data[i], tmpdata[i]);
        }
    }
}

void coupler(const Dims &shape, const Dims &start, const Dims &count, const size_t steps, const adios2::Params &engineParams)
{
    size_t datasize = std::accumulate(count.begin(), count.end(), 1, std::multiplies<size_t>());

    adios2::ADIOS adios(mpiComm, adios2::DebugON);

    adios2::IO x_to_c_io = adios.DeclareIO("x_to_c");
    x_to_c_io.SetEngine("ssc");
    x_to_c_io.SetParameters(engineParams);

    adios2::IO c_to_x_io = adios.DeclareIO("c_to_x");
    c_to_x_io.SetEngine("ssc");
    c_to_x_io.SetParameters(engineParams);

    adios2::IO g_to_c_io = adios.DeclareIO("g_to_c");
    g_to_c_io.SetEngine("ssc");
    g_to_c_io.SetParameters(engineParams);

    adios2::IO c_to_g_io = adios.DeclareIO("c_to_g");
    c_to_g_io.SetEngine("ssc");
    c_to_g_io.SetParameters(engineParams);

    std::vector<float> x_to_c_data;
    std::vector<float> c_to_x_data(datasize);
    std::vector<float> g_to_c_data;
    std::vector<float> c_to_g_data(datasize);

    auto c_to_x_var = c_to_x_io.DefineVariable<float>("c_to_x", shape, start, count);
    auto c_to_g_var = c_to_g_io.DefineVariable<float>("c_to_g", shape, start, count);

    adios2::Engine x_to_c_engine = x_to_c_io.Open("x-to-c", adios2::Mode::Read);
    adios2::Engine c_to_g_engine = x_to_c_io.Open("c-to-g", adios2::Mode::Write);
    adios2::Engine g_to_c_engine = x_to_c_io.Open("g-to-c", adios2::Mode::Read);
    adios2::Engine c_to_x_engine = c_to_x_io.Open("c-to-x", adios2::Mode::Write);

    for (int i = 0; i < steps; ++i)
    {
        x_to_c_engine.BeginStep();
        auto x_to_c_var = x_to_c_io.InquireVariable<float>("x_to_c");
        x_to_c_data.resize(std::accumulate(x_to_c_var.Shape().begin(), x_to_c_var.Shape().end(), 1, std::multiplies<size_t>()));
        x_to_c_engine.Get(x_to_c_var, x_to_c_data.data(), adios2::Mode::Sync);
        VerifyData(x_to_c_data.data(), i, Dims(x_to_c_var.Shape().size(), 0), x_to_c_var.Shape(), x_to_c_var.Shape());
        x_to_c_engine.EndStep();

        c_to_g_engine.BeginStep();
        GenData(c_to_g_data, i, start, count, shape);
        c_to_g_engine.Put(c_to_g_var, c_to_g_data.data(), adios2::Mode::Sync);
        c_to_g_engine.EndStep();

        g_to_c_engine.BeginStep();
        auto g_to_c_var = g_to_c_io.InquireVariable<float>("g_to_c");
        g_to_c_data.resize(std::accumulate(g_to_c_var.Shape().begin(), g_to_c_var.Shape().end(), 1, std::multiplies<size_t>()));
        g_to_c_engine.Get(g_to_c_var, g_to_c_data.data(), adios2::Mode::Sync);
        VerifyData(g_to_c_data.data(), i, Dims(g_to_c_var.Shape().size(), 0), g_to_c_var.Shape(), g_to_c_var.Shape());
        g_to_c_engine.EndStep();

        c_to_x_engine.BeginStep();
        GenData(c_to_x_data, i, start, count, shape);
        c_to_x_engine.Put(c_to_x_var, c_to_x_data.data(), adios2::Mode::Sync);
        c_to_x_engine.EndStep();
    }

    x_to_c_engine.BeginStep();
    x_to_c_engine.Close();
    c_to_g_engine.Close();
    g_to_c_engine.BeginStep();
    g_to_c_engine.Close();
    c_to_x_engine.Close();
}

void xgc(const Dims &shape, const Dims &start, const Dims &count, const size_t steps, const adios2::Params &engineParams)
{
    size_t datasize = std::accumulate(count.begin(), count.end(), 1, std::multiplies<size_t>());

    std::vector<float> x_to_c_data(datasize);
    std::vector<float> c_to_x_data;

    adios2::ADIOS adios(mpiComm, adios2::DebugON);

    adios2::IO x_to_c_io = adios.DeclareIO("x_to_c");
    x_to_c_io.SetEngine("ssc");
    x_to_c_io.SetParameters(engineParams);

    adios2::IO c_to_x_io = adios.DeclareIO("c_to_x");
    c_to_x_io.SetEngine("ssc");
    c_to_x_io.SetParameters(engineParams);

    auto x_to_c_var = x_to_c_io.DefineVariable<float>("x_to_c", shape, start, count);

    adios2::Engine x_to_c_engine = x_to_c_io.Open("x-to-c", adios2::Mode::Write);
    adios2::Engine c_to_x_engine = c_to_x_io.Open("c-to-x", adios2::Mode::Read);

    for (int i = 0; i < steps; ++i)
    {
        x_to_c_engine.BeginStep();
        GenData(x_to_c_data, i, start, count, shape);
        x_to_c_engine.Put(x_to_c_var, x_to_c_data.data(), adios2::Mode::Sync);
        x_to_c_engine.EndStep();

        c_to_x_engine.BeginStep();
        auto c_to_x_var = x_to_c_io.InquireVariable<float>("c_to_x");
        c_to_x_data.resize(std::accumulate(c_to_x_var.Shape().begin(), c_to_x_var.Shape().end(), 1, std::multiplies<size_t>()));
        c_to_x_engine.Get(c_to_x_var, c_to_x_data.data(), adios2::Mode::Sync);
        VerifyData(c_to_x_data.data(), i, Dims(c_to_x_var.Shape().size(), 0), c_to_x_var.Shape(), c_to_x_var.Shape());
        c_to_x_engine.EndStep();
    }

    x_to_c_engine.Close();
    c_to_x_engine.BeginStep();
    c_to_x_engine.Close();
}

void gene(const Dims &shape, const Dims &start, const Dims &count, const size_t steps, const adios2::Params &engineParams)
{
    adios2::ADIOS adios(mpiComm, adios2::DebugON);

    adios2::IO c_to_g_io = adios.DeclareIO("c_to_g");
    c_to_g_io.SetEngine("ssc");
    c_to_g_io.SetParameters(engineParams);

    adios2::IO g_to_c_io = adios.DeclareIO("g_to_c");
    g_to_c_io.SetEngine("ssc");
    g_to_c_io.SetParameters(engineParams);

    auto g_to_c_var = g_to_c_io.DefineVariable<float>("g_to_c", shape, start, count);

    adios2::Engine c_to_g_engine = c_to_g_io.Open("c_to_g", adios2::Mode::Read);
    adios2::Engine g_to_c_engine = g_to_c_io.Open("g_to_c", adios2::Mode::Write);

    size_t datasize = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());
    std::vector<float> c_to_g_data;
    std::vector<float> g_to_c_data(datasize);

    for (int i = 0; i < steps; ++i)
    {
        c_to_g_engine.BeginStep(StepMode::Read, 5);
        auto c_to_g_var = c_to_g_io.InquireVariable<float>("c_to_g");
        c_to_g_data.resize(std::accumulate(c_to_g_var.Shape().begin(), c_to_g_var.Shape().end(), 1, std::multiplies<size_t>()));
        c_to_g_engine.Get(c_to_g_var, c_to_g_data.data(), adios2::Mode::Sync);
        VerifyData(c_to_g_data.data(), i, Dims(shape.size(), 0), shape, shape);
        c_to_g_engine.EndStep();

        g_to_c_engine.BeginStep();
        GenData(g_to_c_data, i, start, count, shape);
        g_to_c_engine.Put(g_to_c_var, g_to_c_data.data(), adios2::Mode::Sync);
        g_to_c_engine.EndStep();
    }

    c_to_g_engine.BeginStep();
    c_to_g_engine.Close();
    g_to_c_engine.Close();
}

TEST_F(SscEngineTest, TestSscXgcPattern)
{

    int worldRank, worldSize;
    Dims start, count, shape;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    if (worldSize < 6)
    {
        return;
    }

    if (worldRank == 0 or worldRank == 1)
    {
        mpiGroup = 0;
    }
    else if (worldRank == 2 or worldRank == 3)
    {
        mpiGroup = 1;
    }
    else
    {
        mpiGroup = 2;
    }

    MPI_Comm_split(MPI_COMM_WORLD, mpiGroup, worldRank, &mpiComm);

    MPI_Comm_rank(mpiComm, &mpiRank);
    MPI_Comm_size(mpiComm, &mpiSize);

    size_t steps = 20;

    if (mpiGroup == 0)
    {
        shape = {(size_t)mpiSize, 10};
        start = {(size_t)mpiRank, 0};
        count = {1, 10};
        adios2::Params engineParams = {{"RendezvousAppCount", "2"}, {"MaxStreamsPerApp", "4"}, {"OpenTimeoutSecs", "3"}};
        std::cout << "Rank " << worldRank << " launched coupler" << std::endl;
        coupler(shape, start, count, steps, engineParams);
    }

    if (mpiGroup == 1)
    {
        shape = {(size_t)mpiSize, 10};
        start = {(size_t)mpiRank, 0};
        count = {1, 10};
        adios2::Params engineParams = {{"RendezvousAppCount", "2"}, {"MaxStreamsPerApp", "4"}, {"OpenTimeoutSecs", "3"}};
        std::cout << "Rank " << worldRank << " launched gene" << std::endl;
        gene(shape, start, shape, steps, engineParams);
    }

    if (mpiGroup == 2)
    {
        shape = {(size_t)mpiSize, 10};
        start = {(size_t)mpiRank, 0};
        count = {1, 10};
        adios2::Params engineParams = {{"RendezvousAppCount", "2"}, {"MaxStreamsPerApp", "4"}, {"OpenTimeoutSecs", "3"}};
        std::cout << "Rank " << worldRank << " launched xgc" << std::endl;
        xgc(shape, start, count, steps, engineParams);
    }

    MPI_Barrier(MPI_COMM_WORLD);
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
