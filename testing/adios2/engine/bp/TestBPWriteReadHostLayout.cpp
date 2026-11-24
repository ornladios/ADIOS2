/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <adios2.h>

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

// SetArrayLayout on host buffers, in any build: RowMajor keeps the declared shape,
// ColumnMajor declares the buffer column-major and the stored shape is reversed,
// and the layout is fixed by the first SetArrayLayout or Put.

std::string engineName; // comes from command line

namespace
{

constexpr size_t Nx = 2;
constexpr size_t Ny = 3;
const adios2::Dims declaredShape{Nx, Ny};
const adios2::Dims reversedShape{Ny, Nx};
const std::vector<float> values{0, 1, 2, 3, 4, 5};

void SetEngine(adios2::IO &io)
{
    io.SetEngine("BP5");
    if (!engineName.empty())
        io.SetEngine(engineName);
}

void Write(const std::string &fname, const adios2::ArrayOrdering layout)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("WriteHostLayout");
    SetEngine(io);
    auto var = io.DefineVariable<float>("r32", declaredShape, {0, 0}, declaredShape);
    var.SetArrayLayout(layout);
    adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
    writer.BeginStep();
    writer.Put(var, values.data(), adios2::Mode::Sync);
    writer.EndStep();
    writer.Close();
}

} // namespace

class BPWRHostLayout : public ::testing::Test
{
};

TEST_F(BPWRHostLayout, RowMajorKeepsShape)
{
    const std::string fname("BPWRHostLayoutRow.bp");
    Write(fname, adios2::ArrayOrdering::RowMajor);

    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("ReadHostLayout");
    SetEngine(io);
    adios2::Engine reader = io.Open(fname, adios2::Mode::Read);
    reader.BeginStep();
    auto var = io.InquireVariable<float>("r32");
    ASSERT_TRUE(var);
    EXPECT_EQ(var.Shape(), declaredShape);
    std::vector<float> in(Nx * Ny);
    reader.Get(var, in.data(), adios2::Mode::Sync);
    reader.EndStep();
    reader.Close();
    EXPECT_EQ(in, values);
}

TEST_F(BPWRHostLayout, ColumnMajorReversesShape)
{
    const std::string fname("BPWRHostLayoutCol.bp");
    Write(fname, adios2::ArrayOrdering::ColumnMajor);

    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("ReadHostLayout");
    SetEngine(io);
    adios2::Engine reader = io.Open(fname, adios2::Mode::Read);
    reader.BeginStep();
    auto var = io.InquireVariable<float>("r32");
    ASSERT_TRUE(var);
    // a row-major reader sees the reversed shape, the bytes stored unchanged
    EXPECT_EQ(var.Shape(), reversedShape);
    EXPECT_EQ(var.Shape(adios2::ArrayOrdering::ColumnMajor), declaredShape);
    std::vector<float> in(Nx * Ny);
    reader.Get(var, in.data(), adios2::Mode::Sync);
    reader.EndStep();
    reader.Close();
    EXPECT_EQ(in, values);
}

TEST_F(BPWRHostLayout, ColumnMajorReadBack)
{
    const std::string fname("BPWRHostLayoutColRead.bp");
    Write(fname, adios2::ArrayOrdering::ColumnMajor);

    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("ReadHostLayout");
    SetEngine(io);
    adios2::Engine reader = io.Open(fname, adios2::Mode::Read);
    reader.BeginStep();
    auto var = io.InquireVariable<float>("r32");
    ASSERT_TRUE(var);
    // columns 1..Ny-1 of the column-major Nx x Ny array, in its own indices
    var.SetSelection({{0, 1}, {Nx, Ny - 1}});
    var.SetArrayLayout(adios2::ArrayOrdering::ColumnMajor);
    std::vector<float> in(Nx * (Ny - 1));
    reader.Get(var, in.data(), adios2::Mode::Sync);
    reader.EndStep();
    reader.Close();
    EXPECT_EQ(in, std::vector<float>({2, 3, 4, 5}));
}

TEST_F(BPWRHostLayout, LayoutIsFixed)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("WriteHostLayout");
    SetEngine(io);

    // Auto is the IO's order, and fixes the layout like any other
    auto var = io.DefineVariable<float>("auto", declaredShape, {0, 0}, declaredShape);
    var.SetArrayLayout(adios2::ArrayOrdering::Auto);
    EXPECT_EQ(var.Shape(), declaredShape);
    EXPECT_THROW(var.SetArrayLayout(adios2::ArrayOrdering::ColumnMajor), std::invalid_argument);

    // the first Put fixes it too
    auto put = io.DefineVariable<float>("put", declaredShape, {0, 0}, declaredShape);
    adios2::Engine writer = io.Open("BPWRHostLayoutFixed.bp", adios2::Mode::Write);
    writer.BeginStep();
    writer.Put(put, values.data(), adios2::Mode::Sync);
    EXPECT_THROW(put.SetArrayLayout(adios2::ArrayOrdering::ColumnMajor), std::invalid_argument);
    writer.EndStep();
    writer.Close();
}

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    int provided;
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
#endif

    ::testing::InitGoogleTest(&argc, argv);
    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    int result = RUN_ALL_TESTS();

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif
    return result;
}
