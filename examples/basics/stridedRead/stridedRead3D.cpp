/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * * stridedRead3D.cpp : example to read from 3D array with a stride
 *
 *  Created on: Feb 13, 2024
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

void writer(adios2::ADIOS &adios, const std::size_t nx, const std::size_t ny, const std::size_t nz)
{
    auto lf_computeTrig = [](const std::size_t nx, const std::size_t ny,
                             const std::size_t nz) -> std::vector<double> {
        const double spx = TWOPI / nx;
        const double spy = TWOPI / ny;
        const double spz = TWOPI / nz;
        std::vector<double> array(nx * ny * nz);
        size_t pos = 0;
        for (size_t i = 0; i < nx; ++i)
        {
            double c = cos(i * spx);
            for (size_t j = 0; j < ny; ++j)
            {
                c += sin(j * spy);
                for (size_t k = 0; k < nz; ++k)
                {
                    array[pos] = c + sin(k * spz);
                    ++pos;
                }
            }
        }
        return array;
    };

    auto lf_computeID = [](const std::size_t nx, const std::size_t ny,
                           const std::size_t nz) -> std::vector<double> {
        std::vector<double> array(nx * ny * nz);
        size_t pos = 0;
        for (size_t i = 0; i < nx; ++i)
        {
            double ci = i * 1.0;
            for (size_t j = 0; j < ny; ++j)
            {
                double c = ci + j * 0.01;
                for (size_t k = 0; k < nz; ++k)
                {
                    array[pos] = c;
                    c += 0.0001;
                    pos++;
                }
            }
        }
        return array;
    };

    adios2::IO io = adios.DeclareIO("stridedRead3D-writer");

    const adios2::Dims shape = {nx, ny, nz};
    adios2::Variable<double> varGlobal3D =
        io.DefineVariable<double>("global3d", shape, {0, 0, 0}, shape);

    adios2::Engine writer = io.Open("stridedRead3D.bp", adios2::Mode::Write);

    const std::vector<double> array = lf_computeID(nx, ny, nz);

    writer.BeginStep();

    // let's write the global array as eight separate writes
    size_t qx[2];
    qx[0] = (nx / 2);
    qx[1] = nx - qx[0];
    size_t ox[2] = {0, qx[0]};

    size_t qy[2];
    qy[0] = (ny / 2);
    qy[1] = ny - qy[0];
    size_t oy[2] = {0, qy[0]};

    size_t qz[2];
    qz[0] = (nz / 2);
    qz[1] = nz - qz[0];
    size_t oz[2] = {0, qz[0]};

    for (size_t x = 0; x < 2; ++x)
    {
        for (size_t y = 0; y < 2; ++y)
        {
            for (size_t z = 0; z < 2; ++z)
            {
                varGlobal3D.SetSelection({{ox[x], oy[y], oz[z]}, {qx[x], qy[y], qz[z]}});
                varGlobal3D.SetMemorySelection({{ox[x], oy[y], oz[z]}, {nx, ny, nz}});
                writer.Put(varGlobal3D, array.data());
            }
        }
    }
    writer.EndStep();
    writer.Close();
}

constexpr double twelfth = 1.0 / 12.0;
const std::vector<double> M3 = {
    0.0,     0.0,         0.0, // 1
    0.0,     twelfth,     0.0, // 2
    0.0,     0.0,         0.0, // 3

    0.0,     twelfth,     0.0,     // 1
    twelfth, 6 * twelfth, twelfth, // 2
    0.0,     twelfth,     0.0,     // 3

    0.0,     0.0,         0.0, // 1
    0.0,     twelfth,     0.0, // 2
    0.0,     0.0,         0.0, // 3
};

void reader(adios2::ADIOS &adios)
{
    adios2::IO io = adios.DeclareIO("stridedRead3D-reader");
    io.SetParameter("Threads", "1");
    adios2::Engine reader = io.Open("stridedRead3D.bp", adios2::Mode::Read);
    reader.BeginStep();

    adios2::DoubleMatrix stencil3D({3, 3, 3}, M3);
    adios2::Dims stride = {2, 2, 2};

    adios2::Variable<double> varGlobal3D = io.InquireVariable<double>("global3d");
    std::vector<double> global3D;
    varGlobal3D.SetSelection(
        {{3, 1, 2},
         {varGlobal3D.Shape()[0] - 3, varGlobal3D.Shape()[1] - 1, varGlobal3D.Shape()[2] - 2}});
    varGlobal3D.SetStride(stride, stencil3D);
    size_t sg = varGlobal3D.SelectionSize();
    global3D.resize(sg);

    auto sel = varGlobal3D.Selection();
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

    reader.Get(varGlobal3D, global3D);

    reader.EndStep();
    reader.Close();

    // write out the result
    {
        adios2::IO io = adios.DeclareIO("stridedRead3D-write-again");
        std::string outname = "global3d-" + std::to_string(stride[0]) + "-" +
                              std::to_string(stride[1]) + "-" + std::to_string(stride[2]);
        adios2::Variable<double> v = io.DefineVariable<double>(outname, sel.second, {0, 0, 0},
                                                               sel.second, adios2::ConstantDims);

        adios2::Engine writer = io.Open("stridedRead3D.bp", adios2::Mode::Append);
        writer.BeginStep();
        writer.Put(v, global3D.data());
        writer.EndStep();
        writer.Close();

        std::cout << "Global array read with stride = {\n  ";
        size_t pos = 0;
        for (size_t i = 0; i < sel.second[0]; ++i)
        {
            // size_t pos = i * sel.second[1];
            for (size_t j = 0; j < sel.second[1]; ++j)
            {
                for (size_t k = 0; k < sel.second[2]; ++k)
                {
                    std::cout << global3D[pos++] << " ";
                }
                std::cout << "\n  ";
            }
            std::cout << "\n  ";
        }
        std::cout << "}" << std::endl;
    }
}

int main(int argc, char *argv[])
{
    try
    {
        constexpr std::size_t nx = 11;
        constexpr std::size_t ny = 7;
        constexpr std::size_t nz = 9;
        adios2::ADIOS adios;
        writer(adios, nx, ny, nz);
        reader(adios);
    }
    catch (const std::exception &e)
    {
        std::cout << "ERROR: ADIOS2 exception: " << e.what() << "\n";
    }

    return 0;
}
