#include <algorithm>
#include <ios>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>

#include <adios2.h>
#if ADIOS2_USE_MPI
#include <mpi.h>
#endif

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

    const size_t Nx = 2, Ny = 3;
    const size_t steps = 2;
    std::vector<float> commonArray(Nx * Ny);
    std::iota(commonArray.begin(), commonArray.end(), 4.0f);

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif

    /* PRIMARY file */
    {
        adios2::IO bpIO = adios.DeclareIO("BPcmp1");

        auto bpFloats = bpIO.DefineVariable<float>("bpFloats", {size * Nx, Ny}, {rank * Nx, 0},
                                                   {Nx, Ny}, adios2::ConstantDims);

        auto bpStep = bpIO.DefineVariable<unsigned int>("bpStep");

        // Create the raw file with the 2D raw array and step information
        adios2::Engine bpWriter = bpIO.Open("TestBPcmp_set1.bp", adios2::Mode::Write);

        for (size_t i = 0; i < steps; i++)
        {
            bpWriter.BeginStep();
            bpWriter.Put(bpFloats, commonArray.data());
            bpWriter.Put(bpStep, static_cast<unsigned int>(i));
            bpWriter.EndStep();

            std::transform(commonArray.begin(), commonArray.end(), commonArray.begin(),
                           [](float &v) { return v + 1; });
        }

        bpWriter.Close();
    }

    /* SECONDARY file */
    {
        adios2::IO bpIO = adios.DeclareIO("BPcmp2");

        auto bpFloats = bpIO.DefineVariable<float>("bpFloats", {size * Nx, Ny}, {rank * Nx, 0},
                                                   {Nx, Ny}, adios2::ConstantDims);

        auto bpStep = bpIO.DefineVariable<int>("bpStep", {1}, {0}, {1});

        // Create the raw file with the 2D raw array and step information
        adios2::Engine bpWriter = bpIO.Open("TestBPcmp_set2.bp", adios2::Mode::Write);

        for (size_t i = 0; i < steps; i++)
        {
            bpWriter.BeginStep();
            bpWriter.Put(bpFloats, commonArray.data());
            bpWriter.Put(bpStep, static_cast<int>(i));
            bpWriter.EndStep();
        }
        bpWriter.BeginStep();
        bpWriter.Put(bpStep, static_cast<int>(steps));
        bpWriter.EndStep();

        bpWriter.Close();
    }

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return 0;
}
