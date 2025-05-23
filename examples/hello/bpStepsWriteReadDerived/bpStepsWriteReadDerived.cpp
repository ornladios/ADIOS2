/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * bpStepsWriteReadDerived.cpp  Simple example of writing and reading two derived variables
 * one that only stores stats and one that stores data
 */
#include <algorithm>
#include <ios>
#include <iostream>
#if ADIOS2_USE_MPI
#include <mpi.h>
#endif
#include <stdexcept>
#include <vector>

#include <adios2.h>

void update_array(std::vector<float> &array, int val)
{
    std::transform(array.begin(), array.end(), array.begin(),
                   [val](float v) -> float { return v + static_cast<float>(val); });
}

void writer(adios2::ADIOS &adios, const std::string &fname, const size_t Nx, unsigned int nSteps,
            int rank, int size)
{
    std::vector<float> simData1(Nx, 0);
    std::vector<float> simData2(Nx, 0);

    adios2::IO bpIO = adios.DeclareIO("WriteIO");

    const adios2::Dims shape{static_cast<size_t>(size * Nx)};
    const adios2::Dims start{static_cast<size_t>(rank * Nx)};
    const adios2::Dims count{Nx};
    auto bpFloats1 = bpIO.DefineVariable<float>("bpFloats1", shape, start, count);
    auto bpFloats2 = bpIO.DefineVariable<float>("bpFloats2", shape, start, count);

    bpIO.DefineDerivedVariable("derived/magnitude",
                               "x = bpFloats1 \n"
                               "y = bpFloats2 \n"
                               "magnitude(x, y)",
                               adios2::DerivedVarType::StatsOnly);
    bpIO.DefineDerivedVariable("derived/sqrt",
                               "x = bpFloats1 \n"
                               "sqrt(x)",
                               adios2::DerivedVarType::StoreData);
    adios2::Engine bpWriter = bpIO.Open(fname, adios2::Mode::Write);

    for (unsigned int step = 0; step < nSteps; ++step)
    {
        bpWriter.BeginStep();
        bpWriter.Put(bpFloats1, simData1.data());
        bpWriter.Put(bpFloats2, simData2.data());
        bpWriter.EndStep();

        // Update values in the simulation data
        update_array(simData1, 5);
        update_array(simData2, 10);
    }

    bpWriter.Close();
    std::cout << "Done writing " << nSteps << " steps" << std::endl;
}

void reader(adios2::ADIOS &adios, const std::string &fname, const size_t Nx, int rank)
{
    adios2::IO bpIO = adios.DeclareIO("ReadIO");

    adios2::Engine bpReader = bpIO.Open(fname, adios2::Mode::Read);

    std::vector<float> magData(Nx, 0);
    std::vector<double> sqrtData(Nx, 0);
    unsigned int step;
    for (step = 0; bpReader.BeginStep() == adios2::StepStatus::OK; ++step)
    {
        auto bpMag = bpIO.InquireVariable<float>("derived/magnitude");
        if (bpMag)
        {
            const adios2::Box<adios2::Dims> sel({{Nx * rank}, {Nx}});
            bpMag.SetSelection(sel);
            bpReader.Get(bpMag, magData.data());
        }
        auto bpSqrt = bpIO.InquireVariable<double>("derived/sqrt");
        if (bpSqrt)
        {
            const adios2::Box<adios2::Dims> sel({{Nx * rank}, {Nx}});
            bpSqrt.SetSelection(sel);
            bpReader.Get(bpSqrt, sqrtData.data());
        }

        bpReader.EndStep();
    }
    bpReader.Close();
    std::cout << "Done reading " << step << " steps" << std::endl;
}

int main(int argc, char *argv[])
{
    int rank, size;

#if ADIOS2_USE_MPI
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#else
    rank = 0;
    size = 1;
#endif

    const std::string filename = "StepsWriteReadDerived.bp";
    const unsigned int nSteps = 10;
    const unsigned int Nx = 60000;
    try
    {
#if ADIOS2_USE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD);
#else
        adios2::ADIOS adios;
#endif

        writer(adios, filename, Nx, nSteps, rank, size);
        reader(adios, filename, Nx, rank);
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "IO System base failure exception, STOPPING PROGRAM "
                     "from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return 0;
}
