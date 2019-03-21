#include <cstdint>

#include <iostream>
#include <numeric>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#ifdef ADIOS2_HAVE_MPI

TEST(ADIOSInterface, MPICommRemoved)
{
    MPI_Comm myComm;
    MPI_Comm_dup(MPI_COMM_WORLD, &myComm);
    adios2::ADIOS adios(myComm);
    adios2::IO io = adios.DeclareIO("TestIO");
    MPI_Comm_free(&myComm);

    adios2::Engine engine = io.Open("test.bp", adios2::Mode::Write);
}

#endif

class ADIOS2_CXX11_API : public ::testing::Test
{
public:
    ADIOS2_CXX11_API()
#ifdef ADIOS2_HAVE_MPI
    : m_Ad(MPI_COMM_WORLD, adios2::DebugON)
#else
    : m_Ad(adios2::DebugON)
#endif
    {
#ifdef ADIOS2_HAVE_MPI
        MPI_Comm_rank(MPI_COMM_WORLD, &m_MpiRank);
        MPI_Comm_size(MPI_COMM_WORLD, &m_MpiSize);
#endif
    }

    adios2::ADIOS m_Ad;
    int m_MpiRank = 0;
    int m_MpiSize = 1;
};

class ADIOS2_CXX11_API_IO : public ADIOS2_CXX11_API
{
public:
    ADIOS2_CXX11_API_IO() : m_Io(m_Ad.DeclareIO("CXX11_API_TestIO")) {}

    adios2::IO m_Io;
};

TEST_F(ADIOS2_CXX11_API_IO, Engine)
{
    m_Io.SetEngine("bpfile");
    EXPECT_EQ(m_Io.EngineType(), "bpfile");

    adios2::Engine engine = m_Io.Open("types.bp", adios2::Mode::Write);
    EXPECT_EQ(engine.Name(), "types.bp");
    EXPECT_EQ(engine.Type(), "BP3");

    EXPECT_EQ(m_Io.EngineType(), "bp"); // FIXME? Is it expected that adios2_open
                                      // changes the engine_type string?
}

TEST_F(ADIOS2_CXX11_API_IO, EngineDefault)
{
    m_Io.SetEngine("");
    EXPECT_EQ(m_Io.EngineType(), "");

    adios2::Engine engine = m_Io.Open("types.bp", adios2::Mode::Write);
    EXPECT_EQ(engine.Name(), "types.bp");
    EXPECT_EQ(engine.Type(), "BP3");

    EXPECT_EQ(m_Io.EngineType(), "bp"); // FIXME? Is it expected that adios2_open
                                      // changes the engine_type string?
}

template <class T>
struct MyData
{
    using Block = std::vector<T>;
    using Box = adios2::Box<adios2::Dims>;

    explicit MyData(const std::vector<Box> &selections)
    : m_Blocks(selections.size()), m_Selections(selections)
    {
        for (int b = 0; b < NBlocks(); ++b)
        {
            m_Blocks[b].resize(selections[b].second[0]);
        }
    }

    size_t NBlocks() const { return m_Selections.size(); }
    size_t Start(int b) const { return m_Selections[b].first[0]; }
    size_t Count(int b) const { return m_Selections[b].second[0]; }
    const Box &Selection(int b) const { return m_Selections[b]; }
    Block &operator[](int b) { return m_Blocks[b]; }

private:
    std::vector<Block> m_Blocks;
    std::vector<Box> m_Selections;
};

template <class T>
struct MyDataView
{
    using Block = T *;
    using Box = adios2::Box<adios2::Dims>;

    explicit MyDataView(const std::vector<Box> &selections)
    : m_Blocks(selections.size()), m_Selections(selections)
    {
    }

    void Place(int b, T *arr) { m_Blocks[b] = arr; }

    size_t NBlocks() const { return m_Selections.size(); }
    size_t Start(int b) const { return m_Selections[b].first[0]; }
    size_t Count(int b) const { return m_Selections[b].second[0]; }
    const Box &Selection(int b) const { return m_Selections[b]; }
    Block operator[](int b) { return m_Blocks[b]; }

private:
    std::vector<Block> m_Blocks;
    std::vector<Box> m_Selections;
};

class ADIOS2_CXX11_API_Put : public ADIOS2_CXX11_API_IO
{
public:
    using T = double;
    using Box = adios2::Box<adios2::Dims>;

    using ADIOS2_CXX11_API_IO::ADIOS2_CXX11_API_IO;

    void SetupDecomposition(size_t Nx)
    {
        m_Nx = Nx;
        m_Shape = {m_MpiSize * Nx};
        m_Selections = {{{m_MpiRank * Nx}, {Nx / 2}},
                        {{m_MpiRank * Nx + Nx / 2}, {Nx / 2}}};
    }

    template <class MyData>
    void PopulateBlock(MyData &myData, int b)
    {
        std::iota(&myData[b][0], &myData[b][myData.Count(b)],
                  T(myData.Start(b)));
    }

    void GenerateOutput(std::string filename)
    {
        adios2::IO io = m_Ad.DeclareIO("CXX11_API_GenerateOutput");
        adios2::Engine engine = io.Open(filename, adios2::Mode::Write);
        adios2::Variable<T> var = io.DefineVariable<T>("var", m_Shape);

        MyData<T> myData(m_Selections);

        for (int b = 0; b < myData.NBlocks(); ++b)
        {
            PopulateBlock(myData, b);

            var.SetSelection(myData.Selection(b));
            engine.Put(var, &myData[b][0], adios2::Mode::Sync);
        }
        engine.Close();

        m_Ad.RemoveIO("CXX11_API_GenerateOutput");
    }

    bool checkOutput(std::string filename)
    {
        if (m_MpiRank != 0)
        {
            return true;
        }
        adios2::IO io = m_Ad.DeclareIO("CXX11_API_CheckOutput");
#ifdef ADIOS2_HAVE_MPI
        adios2::Engine engine =
            io.Open(filename, adios2::Mode::Read, MPI_COMM_SELF);
#else
        adios2::Engine engine = io.Open(filename, adios2::Mode::Read);
#endif
        adios2::Variable<T> var = io.InquireVariable<T>("var");
        adios2::Dims shape = var.Shape();
        std::vector<T> data(shape[0]);
        engine.Get(var, data, adios2::Mode::Sync);
        engine.Close();
        m_Ad.RemoveIO("CXX11_API_CheckOutput");

        std::vector<T> ref(shape[0]);
        std::iota(ref.begin(), ref.end(), T());
        return data == ref;
    }

    size_t m_Nx;
    adios2::Dims m_Shape;
    std::vector<Box> m_Selections;
};

TEST_F(ADIOS2_CXX11_API_Put, MultiBlockPutSync)
{
    SetupDecomposition(10);

    adios2::Engine engine = m_Io.Open("multi_sync.bp", adios2::Mode::Write);
    adios2::Variable<T> var = m_Io.DefineVariable<T>("var", m_Shape);

    MyData<T> myData(m_Selections);

    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        PopulateBlock(myData, b);

        var.SetSelection(myData.Selection(b));
        engine.Put(var, &myData[b][0], adios2::Mode::Sync);
    }
    engine.Close();

    EXPECT_TRUE(checkOutput("multi_sync.bp"));
}

TEST_F(ADIOS2_CXX11_API_Put, MultiBlockPutDeferred)
{
    SetupDecomposition(10);

    adios2::Engine engine = m_Io.Open("multi_deferred.bp", adios2::Mode::Write);
    adios2::Variable<T> var = m_Io.DefineVariable<T>("var", m_Shape);

    MyData<T> myData(m_Selections);

    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        PopulateBlock(myData, b);

        var.SetSelection(myData.Selection(b));
        engine.Put(var, &myData[b][0], adios2::Mode::Deferred);
    }
    engine.Close();

    EXPECT_TRUE(checkOutput("multi_deferred.bp"));
}

TEST_F(ADIOS2_CXX11_API_Put, MultiBlockPutDS)
{
    SetupDecomposition(10);

    adios2::Engine engine = m_Io.Open("multi_ds.bp", adios2::Mode::Write);
    adios2::Variable<T> var = m_Io.DefineVariable<T>("var", m_Shape);

    MyData<T> myData(m_Selections);

    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        PopulateBlock(myData, b);

        var.SetSelection(myData.Selection(b));
        if (b == 0)
        {
            engine.Put(var, &myData[b][0], adios2::Mode::Deferred);
        }
        else
        {
            engine.Put(var, &myData[b][0], adios2::Mode::Sync);
        }
    }
    engine.Close();

    EXPECT_TRUE(checkOutput("multi_ds.bp"));
}

TEST_F(ADIOS2_CXX11_API_Put, MultiBlockPutZeroCopySync)
{
    SetupDecomposition(10);

    adios2::Engine engine = m_Io.Open("multi0_sync.bp", adios2::Mode::Write);
    adios2::Variable<T> var = m_Io.DefineVariable<T>("var", m_Shape);

    MyDataView<T> myData(m_Selections);
    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        var.SetSelection(myData.Selection(b));
        auto span = engine.Put(var);
        myData.Place(b, span.data());
    }

    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        PopulateBlock(myData, b);
    }
    engine.Close();

    EXPECT_TRUE(checkOutput("multi0_sync.bp"));
}

TEST_F(ADIOS2_CXX11_API_Put, MultiBlockPutZeroCopySync2)
{
    SetupDecomposition(10);

    adios2::Engine engine = m_Io.Open("multi0_sync2.bp", adios2::Mode::Write);
    adios2::Variable<T> var = m_Io.DefineVariable<T>("var", m_Shape);

    MyDataView<T> myData(m_Selections);
    for (int b = 0; b < 1; ++b)
    {
        var.SetSelection(myData.Selection(b));
        auto span = engine.Put(var);
        myData.Place(b, span.data());
    }

    for (int b = 0; b < 1; ++b)
    {
        PopulateBlock(myData, b);
    }
    std::vector<T> lastBlock(m_Nx / 2);
    std::iota(lastBlock.begin(), lastBlock.end(),
              T(m_MpiRank * m_Nx + m_Nx / 2));
    var.SetSelection(myData.Selection(1));
    engine.Put(var, lastBlock.data(), adios2::Mode::Deferred);
    engine.Close();

    EXPECT_TRUE(checkOutput("multi0_sync2.bp"));
}

#if 0
TEST_F(ADIOS2_CXX11_API_Put, MultiBlockPutZeroCopySync3)
{
    SetupDecomposition(10);

    adios2::Engine engine = m_Io.Open("multi0_sync3.bp", adios2::Mode::Write);
    adios2::Variable<T> var = m_Io.DefineVariable<T>("var", m_Shape);

    MyDataView<T> myData(m_Selections);
    for (int b = 0; b < 1; ++b)
    {
        var.SetSelection(myData.Selection(b));
        auto span = engine.Put(var);
        myData.Place(b, span.data());
    }

    for (int b = 0; b < 1; ++b)
    {
        PopulateBlock(myData, b);
    }

    engine.Put(var, std::vector<T>{5., 6., 7., 8., 9.}.data(),
               adios2::Mode::Sync);
    engine.Close();

    EXPECT_TRUE(checkOutput("multi0_sync3.bp"));
}
#endif

TEST_F(ADIOS2_CXX11_API_Put, MultiBlockPut2FileGetSyncPutSync)

{
    SetupDecomposition(10);

    GenerateOutput("multi_2f_sync_input.bp");
    adios2::Engine reader =
        m_Io.Open("multi_2f_sync_input.bp", adios2::Mode::Read);
    adios2::Engine writer = m_Io.Open("multi_2f_sync.bp", adios2::Mode::Write);
    adios2::Variable<T> var = m_Io.InquireVariable<T>("var");

    MyData<T> myData(m_Selections);

    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        var.SetSelection(myData.Selection(b));
        reader.Get(var, &myData[b][0], adios2::Mode::Sync);
        writer.Put(var, &myData[b][0], adios2::Mode::Sync);
    }

    reader.Close();
    writer.Close();

    EXPECT_TRUE(checkOutput("multi_2f_sync.bp"));
}

TEST_F(ADIOS2_CXX11_API_Put, MultiBlock2FileGetSyncPutDef)
{
    SetupDecomposition(10);

    GenerateOutput("multi_2f_syncdef_input.bp");

    adios2::Engine reader =
        m_Io.Open("multi_2f_syncdef_input.bp", adios2::Mode::Read);

    adios2::Engine writer = m_Io.Open("multi_2f_syncdef.bp", adios2::Mode::Write);
    adios2::Variable<T> var = m_Io.InquireVariable<T>("var");

    MyData<T> myData(m_Selections);

    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        var.SetSelection(myData.Selection(b));
        reader.Get(var, &myData[b][0], adios2::Mode::Sync);
        writer.Put(var, &myData[b][0], adios2::Mode::Deferred);
    }

    reader.Close();
    writer.Close();

    EXPECT_TRUE(checkOutput("multi_2f_syncdef.bp"));
}

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
