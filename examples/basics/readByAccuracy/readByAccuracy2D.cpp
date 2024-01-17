/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * * readByAccuracy2D.cpp : example to read from 2D array with accuracy set
 * If MGARD MDR operator is available, it will refactor the data on write
 * and support read with a user defined error. Otherwise, reading "normal"
 * data is always fully accurate (error = 0.0)
 *
 *  Created on: Jan 17, 2024
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#include <cstddef>  //std::size_t
#include <iomanip>  // std::setprecision
#include <iostream> // std::cout
#include <limits>   // std::numeric_limits
#include <math.h>
#include <numeric>   //std::iota
#include <stdexcept> //std::exception

#include "adios2/helper/adiosString.h" // AccuracyToString
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

    adios2::IO io = adios.DeclareIO("readByAccuracy2D-writer");

    const adios2::Dims shape = {nx, ny};
    adios2::Variable<double> var =
        io.DefineVariable<double>("data/full", shape, {0, 0}, shape, adios2::ConstantDims);

#ifdef ADIOS2_HAVE_MGARD_MDR1
    adios2::Operator mgardOp = adios.DefineOperator("mgardCompressor", adios2::ops::MDR);
    var.AddOperation(mgardOp, {});
    io.DefineAttribute<std::string>("operator", "Refactored by adios2::ops::MDR", var.Name());
#else
    io.DefineAttribute<std::string>("operator", "none", var.Name());
#endif

    const std::vector<double> array = lf_computeTrig(nx, ny);

    adios2::Engine writer = io.Open("readByAccuracy2D.bp", adios2::Mode::Write);
    writer.BeginStep();
    writer.Put(var, array.data());
    writer.EndStep();
    writer.Close();
}

const std::vector<double> errors = {1.0, 0.1, 0.001, 0.00001};

void reader(adios2::ADIOS &adios)
{
    adios2::IO io = adios.DeclareIO("readByAccuracy2D-reader");
    io.SetParameter("Threads", "1");

    // read data N times into N vectors so that we can output them later
    std::vector<std::vector<double>> arrays(errors.size());
    std::vector<adios2::Accuracy> actualAccuracy(errors.size());

    adios2::Dims varShape;

    {
        adios2::Engine reader = io.Open("readByAccuracy2D.bp", adios2::Mode::Read);
        reader.BeginStep();

        adios2::Variable<double> varFull = io.InquireVariable<double>("data/full");
        varShape = varFull.Shape();

        for (size_t i = 0; i < errors.size(); ++i)
        {
            adios2::Accuracy requestedAccuracy = {errors[i], adios2::Linf_norm, false};
            varFull.SetAccuracy(requestedAccuracy);
            // adios will allocate the vector to fit the data
            // force reading now, so that we can retrieve the accuracy from 'v'
            reader.Get(varFull, arrays[i], adios2::Mode::Sync);
            actualAccuracy[i] = varFull.GetAccuracy();
        }
        reader.EndStep();
        reader.Close();
    }

    // write out the result
    {
        adios2::IO io = adios.DeclareIO("readByAccuracy2D-write-again");
        adios2::Engine writer = io.Open("readByAccuracy2D.bp", adios2::Mode::Append);
        writer.BeginStep();
        for (size_t i = 0; i < errors.size(); ++i)
        {
            std::string outname = "data/" + std::to_string(errors[i]);
            adios2::Variable<double> v =
                io.DefineVariable<double>(outname, varShape, {0, 0}, varShape);
            io.DefineAttribute("accuracy", adios2::helper::AccuracyToString(actualAccuracy[i]),
                               v.Name());

            writer.Put(v, arrays[i].data());
        }
        writer.EndStep();
        writer.Close();
    }
}

int main(int argc, char *argv[])
{
    try
    {
        constexpr std::size_t nx = 1000;
        constexpr std::size_t ny = 1000;
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
