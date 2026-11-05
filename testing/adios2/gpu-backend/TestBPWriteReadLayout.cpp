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

// The layout of a Put buffer comes from the buffer itself, never from the memory
// space it lives in: a Kokkos::View reports its own array_layout, and a raw pointer
// carries no layout at all, so the variable keeps the layout it was declared with.
// Regression tests for https://github.com/ornladios/ADIOS2/issues/5241

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

// A raw pointer has no layout, in either memory space, so the declared row-major
// shape must survive the Put and the bytes must come back in row-major order.
TEST_F(BPWRLayout, RawPointerKeepsDeclaredLayout)
{
    const std::string fname("BPWRLayoutRawPointer.bp");
    auto device = MakeView<Kokkos::LayoutRight, Kokkos::DefaultExecutionSpace::memory_space>("raw");

    {
        adios2::ADIOS adios;
        adios2::IO io = adios.DeclareIO("WriteLayout");
        SetEngine(io);
        auto var = io.DefineVariable<float>("r32", declaredShape, {0, 0}, declaredShape);
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        writer.BeginStep();
        // the raw pointer, not the view, so no layout information reaches ADIOS2
        writer.Put(var, device.data(), adios2::Mode::Sync);
        EXPECT_EQ(var.Shape(), declaredShape);
        writer.EndStep();
        writer.Close();
    }

    adios2::Dims shape;
    std::vector<float> data;
    ReadBack(fname, "r32", shape, data);
    EXPECT_EQ(shape, declaredShape);
    for (size_t i = 0; i < Nx * Ny; ++i)
        EXPECT_EQ(data[i], static_cast<float>(i));
}

// An explicit SetArrayLayout wins over the declared layout, whatever the buffer is.
TEST_F(BPWRLayout, ExplicitLayoutOverridesDeclaredLayout)
{
    const std::string fname("BPWRLayoutExplicit.bp");
    auto device =
        MakeView<Kokkos::LayoutRight, Kokkos::DefaultExecutionSpace::memory_space>("explicit");

    {
        adios2::ADIOS adios;
        adios2::IO io = adios.DeclareIO("WriteLayout");
        SetEngine(io);
        auto var = io.DefineVariable<float>("r32", declaredShape, {0, 0}, declaredShape);
        var.SetArrayLayout(adios2::ArrayOrdering::ColumnMajor);
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        writer.BeginStep();
        writer.Put(var, device.data(), adios2::Mode::Sync);
        writer.EndStep();
        writer.Close();
    }

    adios2::Dims shape;
    std::vector<float> data;
    ReadBack(fname, "r32", shape, data);
    EXPECT_EQ(shape, reversedShape);
}

// Auto means the IO's order, so it leaves the declared shape alone.
TEST_F(BPWRLayout, AutoLayoutIsDeclaredLayout)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("WriteLayout");
    auto var = io.DefineVariable<float>("r32", declaredShape, {0, 0}, declaredShape);
    var.SetArrayLayout(adios2::ArrayOrdering::Auto);
    EXPECT_EQ(var.Shape(), declaredShape);
    EXPECT_THROW(var.SetArrayLayout(adios2::ArrayOrdering::ColumnMajor), std::invalid_argument);
}

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

// A rank-1 view is just a flat buffer: its layout says nothing about the variable's
// dimensions, so the variable keeps its declared layout. LayoutLeft is the Kokkos
// default on GPUs, so this is the common case.
template <class Space>
void FlatViewKeepsDeclaredLayout(const std::string &fname)
{
    Kokkos::View<float *, Kokkos::LayoutLeft, Space> view("flat", Nx * Ny);
    auto host = Kokkos::create_mirror_view(view);
    for (size_t i = 0; i < Nx * Ny; ++i)
        host(i) = static_cast<float>(i);
    Kokkos::deep_copy(view, host);

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
        Kokkos::View<float *, Kokkos::LayoutLeft, Space> in("in", Nx * (Ny - 1));
        reader.Get(var, in, adios2::Mode::Sync);
        reader.EndStep();
        reader.Close();
        auto hostIn = Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace(), in);
        for (size_t i = 0; i < Nx; ++i)
            for (size_t j = 1; j < Ny; ++j)
                EXPECT_EQ(hostIn(i * (Ny - 1) + j - 1), static_cast<float>(i * Ny + j));
    }
}

TEST_F(BPWRLayout, DeviceFlatViewKeepsDeclaredLayout)
{
    FlatViewKeepsDeclaredLayout<Kokkos::DefaultExecutionSpace::memory_space>(
        "BPWRLayoutDeviceFlat.bp");
}

TEST_F(BPWRLayout, HostFlatViewKeepsDeclaredLayout)
{
    FlatViewKeepsDeclaredLayout<Kokkos::HostSpace>("BPWRLayoutHostFlat.bp");
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
