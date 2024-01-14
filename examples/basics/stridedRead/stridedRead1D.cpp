/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * * stridedRead1D.cpp : example to read from 1D array with a stride
 *
 *  Created on: Jan 10, 2024
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

void writer(adios2::ADIOS &adios, const std::size_t nx)
{
    auto lf_compute = [](const std::size_t nx) -> std::vector<double> {
        const double sp = TWOPI / nx;
        std::vector<double> array(nx);
        for (size_t i = 0; i < nx; ++i)
        {
            array[i] = cos(i * sp);
        }
        return array;
    };

    adios2::IO io = adios.DeclareIO("stridedRead1D-writer");

    const adios2::Dims shape = {static_cast<std::size_t>(nx)};
    adios2::Variable<double> varGlobal1D =
        io.DefineVariable<double>("global1d", shape, {0}, shape /*, adios2::ConstantDims*/);

    adios2::Variable<double> varLocal1D =
        io.DefineVariable<double>("local1d", {}, {}, shape, adios2::ConstantDims);

    adios2::Engine writer = io.Open("stridedRead1D.bp", adios2::Mode::Write);

    const std::vector<double> array = lf_compute(nx);

    writer.BeginStep();

    // let's write the global array as two separate writes of halfs
    varGlobal1D.SetSelection({{0}, {nx / 2}});
    writer.Put(varGlobal1D, array.data());
    varGlobal1D.SetSelection({{nx / 2}, {nx - (nx / 2)}});
    writer.Put(varGlobal1D, array.data() + (nx / 2));

    writer.Put(varLocal1D, array.data());
    writer.EndStep();

    writer.Close();
}

void reader(adios2::ADIOS &adios)
{
    adios2::IO io = adios.DeclareIO("stridedRead1D-reader");
    adios2::Engine reader = io.Open("stridedRead1D.bp", adios2::Mode::Read);
    reader.BeginStep();

    adios2::DoubleMatrix stencil1D({3}, {0.25, 0.5, 0.25});

    adios2::Variable<double> varGlobal1D = io.InquireVariable<double>("global1d");
    std::vector<double> global1D;
    varGlobal1D.SetSelection({{10}, {varGlobal1D.Shape()[0] - 20}});
    varGlobal1D.SetStride({2}, stencil1D);
    size_t sg = varGlobal1D.SelectionSize();
    global1D.resize(sg);

    {
        // details about the selection after striding
        auto sel = varGlobal1D.Selection();
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

    reader.Get(varGlobal1D, global1D);

#if 0
    adios2::Variable<double> varLocal1D = io.InquireVariable<double>("local1d");
    std::vector<double> local1D;
    varLocal1D.SetBlockSelection(0);
    double sixteenth = 1.0 / 16;
    /*varLocal1D.SetStride({3},
                         adios2::DoubleMatrix({5}, {sixteenth, 3 * sixteenth, 8 * sixteenth,
                                                    3 * sixteenth, sixteenth}));*/
    varLocal1D.SetStride({3});
    size_t sl = varLocal1D.SelectionSize();
    std::cout << "Local array selection size is " << sl << std::endl;
    local1D.resize(sl);
    reader.Get(varLocal1D, local1D);
#endif

    reader.EndStep();

    std::cout << "Global array read with stride = {" << std::setprecision(2);
    for (auto d : global1D)
    {
        std::cout << d << " ";
    }
    std::cout << "}" << std::endl;

#if 0
    std::cout << "Local array read with stride = {" << std::setprecision(2);
    for (auto d : local1D)
    {
        std::cout << d << " ";
    }
    std::cout << "}" << std::endl;
#endif

    reader.Close();
}

int main(int argc, char *argv[])
{
    try
    {
        constexpr std::size_t nx = 100;
        adios2::ADIOS adios;
        writer(adios, nx);
        reader(adios);
    }
    catch (const std::exception &e)
    {
        std::cout << "ERROR: ADIOS2 exception: " << e.what() << "\n";
    }

    return 0;
}
