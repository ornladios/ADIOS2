/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <adios2.h>

#include <Kokkos_Core.hpp>
#include <adios2/cxx/KokkosView.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

// A Kokkos::View passed to Put/Get reports its own array_layout, which sets the
// variable's layout whatever memory space the view lives in, as long as the view
// has the variable's rank.

namespace
{

std::string engineName;

// A row-major 2x3 array holding 0..5
constexpr size_t Nx = 2;
constexpr size_t Ny = 3;
const adios2::Dims declaredShape{Nx, Ny};
const adios2::Dims reversedShape{Ny, Nx};

template <class Layout, class Space>
Kokkos::View<float **, Layout, Space> MakeView(const std::string &name)
{
    Kokkos::View<float **, Layout, Space> view(name, Nx, Ny);
    auto host = Kokkos::create_mirror_view(view);
    for (size_t i = 0; i < Nx; ++i)
        for (size_t j = 0; j < Ny; ++j)
            host(i, j) = static_cast<float>(i * Ny + j);
    Kokkos::deep_copy(view, host);
    return view;
}

void SetEngine(adios2::IO &io)
{
    io.SetEngine("BP5");
    if (!engineName.empty())
        io.SetEngine(engineName);
}

// Read the variable back as a row-major reader sees it: its shape, and its bytes in
// stored order.
void ReadBack(const std::string &fname, const std::string &varName, adios2::Dims &shape,
              std::vector<float> &data)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("ReadLayout");
    SetEngine(io);
    adios2::Engine reader = io.Open(fname, adios2::Mode::Read);
    reader.BeginStep();
    auto var = io.InquireVariable<float>(varName);
    ASSERT_TRUE(var);
    shape = var.Shape();
    data.resize(Nx * Ny);
    reader.Get(var, data.data(), adios2::Mode::Sync);
    reader.EndStep();
    reader.Close();
}

} // namespace

class BPWRLayout : public ::testing::Test
{
};

// A view reports its own layout, in either memory space: LayoutRight is row-major, so
// the shape is unchanged, while LayoutLeft is column-major, so a row-major reader sees
// the reversed shape and the transposed array.
template <class Layout, class Space>
void ViewLayoutDrivesShape(const std::string &fname, const adios2::Dims &expectedShape)
{
    auto view = MakeView<Layout, Space>("view");

    {
        adios2::ADIOS adios;
        adios2::IO io = adios.DeclareIO("WriteLayout");
        SetEngine(io);
        auto var = io.DefineVariable<float>("r32", declaredShape, {0, 0}, declaredShape);
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        writer.BeginStep();
        writer.Put(var, view, adios2::Mode::Sync);
        writer.EndStep();
        writer.Close();
    }

    adios2::Dims shape;
    std::vector<float> data;
    ReadBack(fname, "r32", shape, data);
    EXPECT_EQ(shape, expectedShape);

    // Either way the bytes are stored unchanged, in the view's own order
    const bool columnMajor = std::is_same<Layout, Kokkos::LayoutLeft>::value;
    for (size_t i = 0; i < Nx; ++i)
        for (size_t j = 0; j < Ny; ++j)
        {
            const size_t offset = columnMajor ? j * Nx + i : i * Ny + j;
            EXPECT_EQ(data[offset], static_cast<float>(i * Ny + j));
        }
}

TEST_F(BPWRLayout, DeviceViewLayoutRight)
{
    ViewLayoutDrivesShape<Kokkos::LayoutRight, Kokkos::DefaultExecutionSpace::memory_space>(
        "BPWRLayoutDeviceRight.bp", declaredShape);
}

TEST_F(BPWRLayout, DeviceViewLayoutLeft)
{
    ViewLayoutDrivesShape<Kokkos::LayoutLeft, Kokkos::DefaultExecutionSpace::memory_space>(
        "BPWRLayoutDeviceLeft.bp", reversedShape);
}

TEST_F(BPWRLayout, HostViewLayoutRight)
{
    ViewLayoutDrivesShape<Kokkos::LayoutRight, Kokkos::HostSpace>("BPWRLayoutHostRight.bp",
                                                                  declaredShape);
}

TEST_F(BPWRLayout, HostViewLayoutLeft)
{
    ViewLayoutDrivesShape<Kokkos::LayoutLeft, Kokkos::HostSpace>("BPWRLayoutHostLeft.bp",
                                                                 reversedShape);
}

// A view whose rank differs from the variable's is just a flat buffer: its layout
// says nothing about the variable's dimensions, so the variable keeps its declared
// layout.
TEST_F(BPWRLayout, FlatViewKeepsDeclaredLayout)
{
    const std::string fname("BPWRLayoutFlat.bp");
    Kokkos::View<float *, Kokkos::LayoutLeft, Kokkos::HostSpace> view("flat", Nx * Ny);
    for (size_t i = 0; i < Nx * Ny; ++i)
        view(i) = static_cast<float>(i);

    {
        adios2::ADIOS adios;
        adios2::IO io = adios.DeclareIO("WriteLayout");
        SetEngine(io);
        auto var = io.DefineVariable<float>("r32", declaredShape, {0, 0}, declaredShape);
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        writer.BeginStep();
        writer.Put(var, view, adios2::Mode::Sync);
        writer.EndStep();
        writer.Close();
    }

    adios2::Dims shape;
    std::vector<float> data;
    ReadBack(fname, "r32", shape, data);
    EXPECT_EQ(shape, declaredShape);
    for (size_t i = 0; i < Nx * Ny; ++i)
        EXPECT_EQ(data[i], static_cast<float>(i));

    // and reading into a flat view is row-major as well
    {
        adios2::ADIOS adios;
        adios2::IO io = adios.DeclareIO("ReadFlat");
        SetEngine(io);
        adios2::Engine reader = io.Open(fname, adios2::Mode::Read);
        reader.BeginStep();
        auto var = io.InquireVariable<float>("r32");
        ASSERT_TRUE(var);
        var.SetSelection({{0, 1}, {Nx, Ny - 1}});
        Kokkos::View<float *, Kokkos::LayoutLeft, Kokkos::HostSpace> in("in", Nx * (Ny - 1));
        reader.Get(var, in, adios2::Mode::Sync);
        reader.EndStep();
        reader.Close();
        for (size_t i = 0; i < Nx; ++i)
            for (size_t j = 1; j < Ny; ++j)
                EXPECT_EQ(in(i * (Ny - 1) + j - 1), static_cast<float>(i * Ny + j));
    }
}

// Two buffers with conflicting layouts remain an error.
TEST_F(BPWRLayout, ConflictingLayoutsThrow)
{
    const std::string fname("BPWRLayoutConflict.bp");
    auto left = MakeView<Kokkos::LayoutLeft, Kokkos::HostSpace>("left");
    auto right = MakeView<Kokkos::LayoutRight, Kokkos::HostSpace>("right");

    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("WriteLayout");
    SetEngine(io);
    auto var = io.DefineVariable<float>("r32", declaredShape, {0, 0}, declaredShape);
    adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
    writer.BeginStep();
    writer.Put(var, left, adios2::Mode::Sync);
    EXPECT_THROW(writer.Put(var, right, adios2::Mode::Sync), std::invalid_argument);
    writer.EndStep();
    writer.Close();
}

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    int provided;
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
#endif
    Kokkos::initialize(argc, argv);

    int result;
    {
        // Execution space instances must be destroyed before Kokkos::finalize()
        Kokkos::DefaultExecutionSpace exe_space;
        std::cout << "Testing on memory space: " << exe_space.name() << std::endl;

        ::testing::InitGoogleTest(&argc, argv);
        if (argc > 1)
        {
            engineName = std::string(argv[1]);
        }
        result = RUN_ALL_TESTS();
    }

    Kokkos::finalize();
#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
}
