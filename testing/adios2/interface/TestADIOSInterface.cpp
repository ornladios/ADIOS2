#include <cstdint>

#include <iostream>
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
    : ad(MPI_COMM_WORLD, adios2::DebugON)
#else
    : ad(adios2::DebugON)
#endif
    {
#ifdef ADIOS2_HAVE_MPI
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif
    }

    adios2::ADIOS ad;
    int rank = 0;
    int size = 1;
};

class ADIOS2_CXX11_API_IO : public ADIOS2_CXX11_API
{
public:
    ADIOS2_CXX11_API_IO() : io(ad.DeclareIO("CXX11_API_TestIO")) {}

    adios2::IO io;
};

TEST_F(ADIOS2_CXX11_API_IO, Engine)
{
    io.SetEngine("bpfile");
    EXPECT_EQ(io.EngineType(), "bpfile");

    adios2::Engine engine = io.Open("types.bp", adios2::Mode::Write);
    EXPECT_EQ(engine.Name(), "types.bp");
    EXPECT_EQ(engine.Type(), "BP3");

    EXPECT_EQ(io.EngineType(), "bp"); // FIXME? Is it expected that adios2_open
                                      // changes the engine_type string?
}

TEST_F(ADIOS2_CXX11_API_IO, EngineDefault)
{
    io.SetEngine("");
    EXPECT_EQ(io.EngineType(), "");

    adios2::Engine engine = io.Open("types.bp", adios2::Mode::Write);
    EXPECT_EQ(engine.Name(), "types.bp");
    EXPECT_EQ(engine.Type(), "BP3");

    EXPECT_EQ(io.EngineType(), "bp"); // FIXME? Is it expected that adios2_open
                                      // changes the engine_type string?
}

template <class T>
struct MyData
{
    using Block = std::vector<T>;
    using Box = adios2::Box<adios2::Dims>;

    MyData(const std::vector<Box> &selections)
    : m_Blocks(selections.size()), m_Selections(selections)
    {
        for (int b = 0; b < nBlocks(); ++b)
        {
            m_Blocks[b].resize(selections[b].second[0]);
        }
    }

    size_t nBlocks() const { return m_Selections.size(); }
    size_t start(int b) const { return m_Selections[b].first[0]; }
    size_t count(int b) const { return m_Selections[b].second[0]; }
    const Box &selection(int b) const { return m_Selections[b]; }
    Block &operator[](int b) { return m_Blocks[b]; }

private:
    std::vector<Block> m_Blocks;
    std::vector<Box> m_Selections;
};

template <class T>
struct MyDataView
{
    using Block = T*;
    using Box = adios2::Box<adios2::Dims>;

    MyDataView(const std::vector<Box> &selections)
    : m_Blocks(selections.size()), m_Selections(selections)
    {
    }

    void place(int b, T* arr)
    {
      m_Blocks[b] = arr;
    }

    size_t nBlocks() const { return m_Selections.size(); }
    size_t start(int b) const { return m_Selections[b].first[0]; }
    size_t count(int b) const { return m_Selections[b].second[0]; }
    const Box &selection(int b) const { return m_Selections[b]; }
    Block operator[](int b) { return m_Blocks[b]; }

private:
    std::vector<Block> m_Blocks;
    std::vector<Box> m_Selections;
};

template <class MyData>
void PopulateBlock(MyData &myData, int b)
{
    auto &&block = myData[b];

    for (size_t i = 0; i < myData.count(b); i++)
    {
        block[i] = i + myData.start(b);
    }
}

TEST_F(ADIOS2_CXX11_API_IO, MultiBlockPutSync)
{
    using T = double;
    using Box = MyData<T>::Box;
    using Block = MyData<T>::Block;
    const size_t Nx = 10;
    const adios2::Dims shape = {size * Nx};
    std::vector<Box> selections = {
        {{rank * Nx}, {Nx / 2}}, {{rank * Nx + Nx / 2}, {Nx / 2}},
    };

    adios2::Engine engine = io.Open("multi_sync.bp", adios2::Mode::Write);
    adios2::Variable<T> var = io.DefineVariable<T>("var", shape);

    MyData<T> myData(selections);

    for (int b = 0; b < myData.nBlocks(); ++b)
    {
        PopulateBlock(myData, b);

        var.SetSelection(myData.selection(b));
        engine.Put(var, &myData[b][0], adios2::Mode::Sync);
    }
    engine.Close();
}

TEST_F(ADIOS2_CXX11_API_IO, MultiBlockPutDeferred)
{
    using T = double;
    using Box = MyData<T>::Box;
    using Block = MyData<T>::Block;
    const size_t Nx = 10;
    const adios2::Dims shape = {size * Nx};
    std::vector<Box> selections = {
        {{rank * Nx}, {Nx / 2}}, {{rank * Nx + Nx / 2}, {Nx / 2}},
    };

    adios2::Engine engine = io.Open("multi_deferred.bp", adios2::Mode::Write);
    adios2::Variable<T> var = io.DefineVariable<T>("var", shape);

    MyData<T> myData(selections);

    for (int b = 0; b < myData.nBlocks(); ++b)
    {
        PopulateBlock(myData, b);

        var.SetSelection(myData.selection(b));
        engine.Put(var, &myData[b][0], adios2::Mode::Deferred);
    }
    engine.Close();
}

TEST_F(ADIOS2_CXX11_API_IO, MultiBlockPutDS)
{
    using T = double;
    using Box = MyData<T>::Box;
    using Block = MyData<T>::Block;
    const size_t Nx = 10;
    const adios2::Dims shape = {size * Nx};
    std::vector<Box> selections = {
        {{rank * Nx}, {Nx / 2}}, {{rank * Nx + Nx / 2}, {Nx / 2}},
    };

    adios2::Engine engine = io.Open("multi_ds.bp", adios2::Mode::Write);
    adios2::Variable<T> var = io.DefineVariable<T>("var", shape);

    MyData<T> myData(selections);

    for (int b = 0; b < myData.nBlocks(); ++b)
    {
        PopulateBlock(myData, b);

        var.SetSelection(myData.selection(b));
	if (b == 0) {
	  engine.Put(var, &myData[b][0], adios2::Mode::Deferred);
	} else {
	  engine.Put(var, &myData[b][0], adios2::Mode::Sync);
	}
    }
    engine.Close();
}

TEST_F(ADIOS2_CXX11_API_IO, MultiBlockPutZeroCopySync)
{
    using T = double;
    using Box = MyDataView<T>::Box;
    using Block = MyDataView<T>::Block;
    const size_t Nx = 10;
    const adios2::Dims shape = {size * Nx};
    std::vector<Box> selections = {
        {{rank * Nx}, {Nx / 2}}, {{rank * Nx + Nx / 2}, {Nx / 2}},
    };

    adios2::Engine engine = io.Open("multi0_sync.bp", adios2::Mode::Write);
    adios2::Variable<T> var = io.DefineVariable<T>("var", shape);

    MyDataView<T> myData(selections);
    for (int b = 0; b < myData.nBlocks(); ++b)
    {
        var.SetSelection(myData.selection(b));
        auto span = engine.Put(var);
        myData.place(b, span.data());
    }

    for (int b = 0; b < myData.nBlocks(); ++b)
    {
        PopulateBlock(myData, b);
        //engine.Put(var, &myData[b][0], adios2::Mode::Sync);
    }
    engine.Close();
}

TEST_F(ADIOS2_CXX11_API_IO, MultiBlockPutZeroCopySync2)
{
    using T = double;
    using Box = MyDataView<T>::Box;
    using Block = MyDataView<T>::Block;
    const size_t Nx = 10;
    const adios2::Dims shape = {size * Nx};
    std::vector<Box> selections = {
        {{rank * Nx}, {Nx / 2}}, {{rank * Nx + Nx / 2}, {Nx / 2}},
    };

    io.SetParameter("MaxBufferSize", "50");
    adios2::Engine engine = io.Open("multi0_sync2.bp", adios2::Mode::Write);
    adios2::Variable<T> var = io.DefineVariable<T>("var", shape);

    MyDataView<T> myData(selections);
    for (int b = 0; b < myData.nBlocks(); ++b)
    {
        var.SetSelection(myData.selection(b));
        auto span = engine.Put(var);
        myData.place(b, span.data());
    }

    for (int b = 0; b < myData.nBlocks(); ++b)
    {
        PopulateBlock(myData, b);
        //engine.Put(var, &myData[b][0], adios2::Mode::Sync);
    }
    engine.Close();
}

TEST_F(ADIOS2_CXX11_API_IO, MultiBlockPutZeroCopySync3)
{
    using T = double;
    using Box = MyDataView<T>::Box;
    using Block = MyDataView<T>::Block;
    const size_t Nx = 10;
    const adios2::Dims shape = {size * Nx};
    std::vector<Box> selections = {
        {{rank * Nx}, {Nx / 2}}, {{rank * Nx + Nx / 2}, {Nx / 2}},
    };

    adios2::Engine engine = io.Open("multi0_sync3.bp", adios2::Mode::Write);
    adios2::Variable<T> var = io.DefineVariable<T>("var", shape);

    MyDataView<T> myData(selections);
    for (int b = 0; b < 1; ++b)
    {
        var.SetSelection(myData.selection(b));
        auto span = engine.Put(var);
        myData.place(b, span.data());
    }

    for (int b = 0; b < 1; ++b)
    {
        PopulateBlock(myData, b);
        //engine.Put(var, &myData[b][0], adios2::Mode::Sync);
    }

    engine.Put(var, std::vector<T>{5., 6., 7., 8., 9.}.data(), adios2::Mode::Sync);
    engine.Close();
}

TEST_F(ADIOS2_CXX11_API_IO, MultiBlockPutTwoFilesSync)
{
    using T = double;
    using Box = MyData<T>::Box;
    using Block = MyData<T>::Block;
    const size_t Nx = 10;
    const adios2::Dims shape = {size * Nx};
    std::vector<Box> selections = {
        {{rank * Nx}, {Nx / 2}}, {{rank * Nx + Nx / 2}, {Nx / 2}},
    };

    adios2::Engine reader = io.Open("multi_sync.bp", adios2::Mode::Read);
    adios2::Engine writer = io.Open("multi_2f.bp", adios2::Mode::Write);
    adios2::Variable<T> var = io.InquireVariable<T>("var");

    MyData<T> myData(selections);

    for (int b = 0; b < myData.nBlocks(); ++b)
    {
      var.SetSelection(myData.selection(b));
      reader.Get(var, &myData[b][0], adios2::Mode::Sync);
      writer.Put(var, &myData[b][0], adios2::Mode::Deferred);
    }

    reader.Close();
    writer.Close();
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
