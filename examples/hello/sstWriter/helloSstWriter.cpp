/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloSstWriter.cpp
 *
 *  Created on: Aug 17, 2017
 *      Author: Greg Eisenhauer
 */

#include <iostream>
#include <numeric>
#include <vector>

#include <adios2.h>

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

int mpiRank = 0;
int mpiSize = 1;

template <class T>
void Dump(std::vector<T> &v)
{
    for (int i = 0; i < mpiSize; ++i)
    {
        MPI_Barrier(MPI_COMM_WORLD);
        if (mpiRank == i)
        {
            std::cout << "Dumping data from Rank " << mpiRank << ": "
                      << std::endl;
            for (const auto &i : v)
            {
                std::cout << i << " ";
            }
            std::cout << std::endl;
        }
    }
}

int main(int argc, char *argv[])
{

#ifdef ADIOS2_HAVE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    std::vector<size_t> dataShape;
    for (int i = 1; i < argc; ++i)
    {
        dataShape.push_back(atoi(argv[i]));
    }

    if (dataShape.empty())
    {
        dataShape.push_back(10);
    }

    size_t dataSize = std::accumulate(dataShape.begin(), dataShape.end(), 1,
                                      std::multiplies<size_t>());
    std::vector<float> myFloats(dataSize);

    for (size_t i = 0; i < dataSize; ++i)
    {
        myFloats[i] = 100000.0 * mpiRank + i;
    }

    try
    {
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
        adios2::IO &sstIO = adios.DeclareIO("myIO");
        sstIO.SetEngine("Sst");
        sstIO.SetParameters({{"BPmarshal", "yes"}, {"FFSmarshal", "no"}});

        std::vector<size_t> shape = dataShape;
        shape[0] *= mpiSize;
        std::vector<size_t> start = dataShape;
        start[0] *= mpiRank;
        for (size_t i = 1; i < start.size(); ++i)
        {
            start[i] = 0;
        }
        std::vector<size_t> count = dataShape;

        // Define variable and local size
        auto bpFloats =
            sstIO.DefineVariable<float>("bpFloats", shape, start, count);

        // Create engine smart pointer to Sst Engine due to polymorphism,
        // Open returns a smart pointer to Engine containing the Derived class
        adios2::Engine &sstWriter = sstIO.Open("helloSst", adios2::Mode::Write);

        Dump(myFloats);
        sstWriter.BeginStep();
        sstWriter.PutSync<float>(bpFloats, myFloats.data());
        sstWriter.EndStep();
        sstWriter.Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM from rank "
                  << mpiRank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout
            << "IO System base failure exception, STOPPING PROGRAM from rank "
            << mpiRank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank " << mpiRank
                  << "\n";
        std::cout << e.what() << "\n";
    }

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return 0;
}
