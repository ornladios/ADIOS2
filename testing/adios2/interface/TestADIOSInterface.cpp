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

    EXPECT_EQ(m_Io.EngineType(),
              "bp"); // FIXME? Is it expected that adios2_open
                     // changes the engine_type string?
}

TEST_F(ADIOS2_CXX11_API_IO, EngineDefault)
{
    m_Io.SetEngine("");
    EXPECT_EQ(m_Io.EngineType(), "");

    adios2::Engine engine = m_Io.Open("types.bp", adios2::Mode::Write);
    EXPECT_EQ(engine.Name(), "types.bp");
    EXPECT_EQ(engine.Type(), "BP3");

    EXPECT_EQ(m_Io.EngineType(),
              "bp"); // FIXME? Is it expected that adios2_open
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

// ----------------------------------------------------------------------
// ADIOS2_CXX11_API_MultiBlock

template <typename _DataType, adios2::Mode _PutMode>
struct CaseMultiBlock
{
    using DataType = _DataType;
    static const adios2::Mode PutMode = _PutMode;
};

using MultiBlockTypes =
    ::testing::Types<CaseMultiBlock<double, adios2::Mode::Sync>,
                     CaseMultiBlock<int64_t, adios2::Mode::Sync>,
                     CaseMultiBlock<double, adios2::Mode::Deferred>,
                     CaseMultiBlock<int64_t, adios2::Mode::Deferred>>;

template <typename TypeParam>
class ADIOS2_CXX11_API_MultiBlock : public ADIOS2_CXX11_API_IO
{
public:
    using DataType = typename TypeParam::DataType;
    using Box = adios2::Box<adios2::Dims>;

    using ADIOS2_CXX11_API_IO::ADIOS2_CXX11_API_IO;

    void SetUp() override
    {
        m_Shape = {m_MpiSize * m_Nx};
        m_Selections = {{{m_MpiRank * m_Nx}, {m_Nx / 2}},
                        {{m_MpiRank * m_Nx + m_Nx / 2}, {m_Nx / 2}}};
    }

    template <class MyData>
    void PopulateBlock(MyData &myData, int b)
    {
        std::iota(&myData[b][0], &myData[b][myData.Count(b)],
                  DataType(myData.Start(b)));
    }

    void GenerateOutput(std::string filename)
    {
        auto io = m_Ad.DeclareIO("CXX11_API_GenerateOutput");
        auto engine = io.Open(filename, adios2::Mode::Write);
        auto var = io.template DefineVariable<DataType>("var", m_Shape);

        MyData<DataType> myData(m_Selections);

        for (int b = 0; b < myData.NBlocks(); ++b)
        {
            PopulateBlock(myData, b);

            var.SetSelection(myData.Selection(b));
            engine.Put(var, &myData[b][0], adios2::Mode::Sync);
        }
        engine.Close();

        m_Ad.RemoveIO("CXX11_API_GenerateOutput");
    }

    void CheckOutput(std::string filename)
    {
        if (m_MpiRank != 0)
        {
            return;
        }
        auto io = m_Ad.DeclareIO("CXX11_API_CheckOutput");
#ifdef ADIOS2_HAVE_MPI
        auto engine = io.Open(filename, adios2::Mode::Read, MPI_COMM_SELF);
#else
        auto engine = io.Open(filename, adios2::Mode::Read);
#endif
        auto var = io.template InquireVariable<DataType>("var");
        auto shape = var.Shape();
        std::vector<DataType> data(shape[0]);
        engine.Get(var, data, adios2::Mode::Sync);
        engine.Close();
        m_Ad.RemoveIO("CXX11_API_CheckOutput");

        std::vector<DataType> ref(shape[0]);
        std::iota(ref.begin(), ref.end(), DataType());
        EXPECT_EQ(data, ref);
    }

    size_t m_Nx = 10;
    adios2::Dims m_Shape;
    std::vector<Box> m_Selections;
};

TYPED_TEST_CASE(ADIOS2_CXX11_API_MultiBlock, MultiBlockTypes);

TYPED_TEST(ADIOS2_CXX11_API_MultiBlock, Put)
{
    using T = typename TypeParam::DataType;

    std::string filename = "multi_put.bp";
    auto writer = this->m_Io.Open(filename, adios2::Mode::Write);
    auto var = this->m_Io.template DefineVariable<T>("var", this->m_Shape);
    MyData<T> myData(this->m_Selections);

    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        this->PopulateBlock(myData, b);
        var.SetSelection(myData.Selection(b));
        writer.Put(var, &myData[b][0], TypeParam::PutMode);
    }

    writer.Close();
    this->CheckOutput(filename);
}

TYPED_TEST(ADIOS2_CXX11_API_MultiBlock, PutMixed)
{
    using T = typename TestFixture::DataType;

    std::string filename = "multi_putmixed.bp";
    auto writer = this->m_Io.Open(filename, adios2::Mode::Write);
    auto var = this->m_Io.template DefineVariable<T>("var", this->m_Shape);
    MyData<T> myData(this->m_Selections);

    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        this->PopulateBlock(myData, b);

        var.SetSelection(myData.Selection(b));
        if (b == 0)
        {
            writer.Put(var, &myData[b][0], adios2::Mode::Deferred);
        }
        else
        {
            writer.Put(var, &myData[b][0], adios2::Mode::Sync);
        }
    }

    writer.Close();
    this->CheckOutput(filename);
}

// this one doesn't actually use the PutMode param
TYPED_TEST(ADIOS2_CXX11_API_MultiBlock, PutZeroCopy)
{
    using T = typename TestFixture::DataType;

    std::string filename = "multi_putzerocopy.bp";
    auto writer = this->m_Io.Open(filename, adios2::Mode::Write);
    auto var = this->m_Io.template DefineVariable<T>("var", this->m_Shape);
    MyDataView<T> myData(this->m_Selections);

#if 0
    // keeping this as an example on how not to use Span
    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        var.SetSelection(myData.Selection(b));
        auto span = writer.Put(var);
        myData.Place(b, span.data());
    }
#else
    std::vector<typename adios2::Variable<T>::Span> spans;
    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        var.SetSelection(myData.Selection(b));
        spans.push_back(writer.Put(var));
    }
    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        myData.Place(b, spans[b].data());
    }
#endif
    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        this->PopulateBlock(myData, b);
    }

    writer.Close();
    this->CheckOutput(filename);
}

// writes last block using regular Put(Sync/Deferred)
TYPED_TEST(ADIOS2_CXX11_API_MultiBlock, PutZeroCopyMixed)
{
    using T = typename TestFixture::DataType;

    std::string filename = "multi_putzerocopymixed.bp";
    auto writer = this->m_Io.Open(filename, adios2::Mode::Write);
    auto var = this->m_Io.template DefineVariable<T>("var", this->m_Shape);

    MyDataView<T> myData(this->m_Selections);
    std::unique_ptr<T[]> lastBlock;
    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        if (b < 1)
        {
            var.SetSelection(myData.Selection(b));
            auto span = writer.Put(var);
            myData.Place(b, span.data());
        }
        else
        {
            lastBlock.reset(new T[myData.Count(b)]);
            myData.Place(b, lastBlock.get());
        }
    }

    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        this->PopulateBlock(myData, b);
    }
    var.SetSelection(myData.Selection(1));
    writer.Put(var, lastBlock.get(), TypeParam::PutMode);

    writer.Close();
    this->CheckOutput(filename);
}

TYPED_TEST(ADIOS2_CXX11_API_MultiBlock, Put2File)
{
    using T = typename TypeParam::DataType;

    this->GenerateOutput("multi_2f_input.bp");

    std::string filename = "multi_put2file.bp";
    auto writer = this->m_Io.Open(filename, adios2::Mode::Write);
    auto reader = this->m_Io.Open("multi_2f_input.bp", adios2::Mode::Read);
    auto var = this->m_Io.template InquireVariable<T>("var");

    MyData<T> myData(this->m_Selections);

    for (int b = 0; b < myData.NBlocks(); ++b)
    {
        var.SetSelection(myData.Selection(b));
        reader.Get(var, &myData[b][0], adios2::Mode::Sync);
        writer.Put(var, &myData[b][0], TypeParam::PutMode);
    }

    reader.Close();
    writer.Close();
    this->CheckOutput(filename);
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
