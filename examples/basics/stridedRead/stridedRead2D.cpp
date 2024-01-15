/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * * stridedRead2D.cpp : example to read from 2D array with a stride
 *
 *  Created on: Jan 14, 2024
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#include <cstddef>  //std::size_t
#include <iomanip>  // std::setprecision
#include <iostream> // std::cout
#include <limits>   // std::numeric_limits
#include <math.h>
#include <numeric>   //std::iota
#include <stdexcept> //std::exception

#include <adios2.h>

constexpr double TWOPI = 2.0 * M_PI;

void writer(adios2::ADIOS &adios, const std::size_t nx, const std::size_t ny)
{
    auto lf_computeTrig = [](const std::size_t nx, const std::size_t ny) -> std::vector<double> {
        const double sp = TWOPI / nx;
        std::vector<double> array(nx * ny);
        size_t pos = 0;
        for (size_t i = 0; i < nx; ++i)
        {
            double c = cos(i * sp);
            for (size_t j = 0; j < ny; ++j)
            {
                array[pos] = c + sin(j * sp);
                ++pos;
            }
        }
        return array;
    };

    auto lf_computeID = [](const std::size_t nx, const std::size_t ny) -> std::vector<double> {
        std::vector<double> array(nx * ny);
        size_t pos = 0;
        for (size_t i = 0; i < nx; ++i)
        {
            double c = i * 1.0;
            for (size_t j = 0; j < ny; ++j)
            {
                array[pos] = c;
                c += 0.01;
                pos++;
            }
        }
        return array;
    };

    adios2::IO io = adios.DeclareIO("stridedRead2D-writer");

    const adios2::Dims shape = {nx, ny};
    adios2::Variable<double> varGlobal2D =
        io.DefineVariable<double>("global2d", shape, {0, 0}, shape /*, adios2::ConstantDims*/);

    adios2::Engine writer = io.Open("stridedRead2D.bp", adios2::Mode::Write);

    const std::vector<double> array = lf_computeID(nx, ny);

    writer.BeginStep();

    // let's write the global array as four separate writes of quarters
    size_t qx1 = (nx / 2);
    size_t qx2 = nx - qx1;
    size_t qy1 = (ny / 2);
    size_t qy2 = ny - qy1;

    varGlobal2D.SetSelection({{0, 0}, {qx1, qx2}});
    varGlobal2D.SetMemorySelection({{0, 0}, {nx, ny}});
    writer.Put(varGlobal2D, array.data());

    varGlobal2D.SetSelection({{0, qy1}, {qx1, qy2}});
    varGlobal2D.SetMemorySelection({{0, qy1}, {nx, ny}});
    writer.Put(varGlobal2D, array.data());

    varGlobal2D.SetSelection({{qx1, 0}, {qx2, qx2}});
    varGlobal2D.SetMemorySelection({{qx1, 0}, {nx, ny}});
    writer.Put(varGlobal2D, array.data());

    varGlobal2D.SetSelection({{qx1, qy1}, {qx2, qy2}});
    varGlobal2D.SetMemorySelection({{qx1, qy1}, {nx, ny}});
    writer.Put(varGlobal2D, array.data());

    writer.EndStep();

    writer.Close();
}

constexpr double sixteenth = 1.0 / 16;
constexpr double eighth = 1.0 / 8;
const std::vector<double> M2 = {
    0.0,    eighth,     0.0,    // 1
    eighth, 4 * eighth, eighth, // 2
    0.0,    eighth,     0.0,    // 3
};

void reader(adios2::ADIOS &adios)
{
    adios2::IO io = adios.DeclareIO("stridedRead2D-reader");
    io.SetParameter("Threads", "1");
    adios2::Engine reader = io.Open("stridedRead2D.bp", adios2::Mode::Read);
    reader.BeginStep();

    adios2::DoubleMatrix stencil2D({3, 3}, M2);

    adios2::Variable<double> varGlobal2D = io.InquireVariable<double>("global2d");
    std::vector<double> global2D;
    varGlobal2D.SetSelection(
        {{11, 10}, {varGlobal2D.Shape()[0] - 20, varGlobal2D.Shape()[1] - 20}});
    varGlobal2D.SetStride({2, 3}, stencil2D);
    size_t sg = varGlobal2D.SelectionSize();
    global2D.resize(sg);

    auto sel = varGlobal2D.Selection();
    {
        // details about the selection after striding
        std::cout << "Global array selection: size is " << sg << " start = { ";
        for (auto s : sel.first)
        {
            std::cout << s << " ";
        }
        std::cout << "}, count = { ";
        for (auto s : sel.second)
        {
            std::cout << s << " ";
        }
        std::cout << "}" << std::endl;
    }

    reader.Get(varGlobal2D, global2D);

    reader.EndStep();

    std::cout << "Global array read with stride = {\n  ";
    size_t pos = 0;
    for (size_t i = 0; i < sel.second[0]; ++i)
    {
        // size_t pos = i * sel.second[1];
        for (size_t j = 0; j < sel.second[1]; ++j)
        {
            std::cout << global2D[pos++] << " ";
        }
        std::cout << "\n  ";
    }
    std::cout << "}" << std::endl;

    reader.Close();
}

int main(int argc, char *argv[])
{
    try
    {
        constexpr std::size_t nx = 100;
        constexpr std::size_t ny = 100;
        adios2::ADIOS adios;
        writer(adios, nx, ny);
        reader(adios);
    }
    catch (const std::exception &e)
    {
        std::cout << "ERROR: ADIOS2 exception: " << e.what() << "\n";
    }

    return 0;
}
