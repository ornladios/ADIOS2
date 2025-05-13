/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <mpi.h>
#include <stdexcept> //std::invalid_argument std::exception
#include <string>
#include <vector>

#include <adios2.h>

int main(int argc, char *argv[])
{
    std::string fname;
    std::string engine;

    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc > 1)
    {
        engine = std::string(argv[1]);
    }
    if (argc > 2)
    {
        fname = std::string(argv[2]);
    }

    std::vector<float> myFloats = {
        0 + 10.0f * rank, 1 + 10.0f * rank, 2 + 10.0f * rank, 3 + 10.0f * rank, 4 + 10.0f * rank,
        5 + 10.0f * rank, 6 + 10.0f * rank, 7 + 10.0f * rank, 8 + 10.0f * rank, 9 + 10.0f * rank};
    const std::size_t Nx = myFloats.size();

    try
    {
        adios2::ADIOS adios(MPI_COMM_WORLD);

        if (rank == 0)
            std::cout << "Writing output file " << fname << " with " << size
                      << " MPI ranks and engine " << engine << std::endl;

        adios2::IO bpIO = adios.DeclareIO("BPFile_N2N");

        adios2::Variable<float> bpFloats = bpIO.DefineVariable<float>(
            "bpFloats", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        bpIO.SetEngine(engine);

        bpIO.DefineAttribute<std::string>("Single_String", "File generated with ADIOS2");

        std::vector<std::string> myStrings = {"one", "two", "three"};
        bpIO.DefineAttribute<std::string>("Array_of_Strings", myStrings.data(), myStrings.size());

        bpIO.DefineAttribute<double>("Attr_Double", 0.f);
        std::vector<double> myDoubles = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        bpIO.DefineAttribute<double>("Array_of_Doubles", myDoubles.data(), myDoubles.size());

        adios2::Engine bpWriter = bpIO.Open(fname, adios2::Mode::Write);

        bpWriter.Put<float>(bpFloats, myFloats.data());

        bpWriter.Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "IO System base failure exception, STOPPING PROGRAM from rank " << rank
                  << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }

    MPI_Finalize();

    return 0;
}
