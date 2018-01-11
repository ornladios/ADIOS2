/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 *  Created on: Jan 2018
 *      Author: Norbert Podhorszki
 */

#include <chrono>
#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <thread>
#include <vector>
#include <utility>
#include <numeric>

#include <gtest/gtest.h>

#include <adios2.h>

static int numprocs, wrank;


struct RunParams
{
    unsigned int npx_w;
    unsigned int npy_w;
    unsigned int npx_r;
    unsigned int npy_r;
    RunParams(unsigned int xw, unsigned int yw, unsigned int xr, unsigned int yr): npx_w{xw}, npy_w{yw}, npx_r{xr}, npy_r{yr} {};
};

/* This function is executed by INSTANTIATE_TEST_CASE_P
   before main() and MPI_Init()!!! */
std::vector<RunParams> CreateRunParams()
{
    std::vector<RunParams> params;
    // 2 process test
    params.push_back(RunParams(1,1,  1,1));
    // 3 process tests
    params.push_back(RunParams(2,1,  1,1));
    params.push_back(RunParams(1,2,  1,1));
    params.push_back(RunParams(1,1,  2,1));
    params.push_back(RunParams(1,1,  1,2));

    return params;
}


class TestInSituMPIWriteRead : public ::testing::TestWithParam<RunParams>
{
    public:
        TestInSituMPIWriteRead() = default;
        const std::string streamName = "TestStream";

        int MainWriters(MPI_Comm comm, unsigned int npx, unsigned int npy, int steps, unsigned int sleeptime)
        {
            int rank, nproc;
            MPI_Comm_rank(comm, &rank);
            MPI_Comm_size(comm, &nproc);
            if (!rank) 
            {
                std::cout << "There are " << nproc << "Writers" << std::endl;
            }
            unsigned int ndx = 5;
            unsigned int ndy = 6;
            unsigned int gndx = npx*ndx; 
            unsigned int gndy = npy*ndy; 
            unsigned int posx = rank % npx; 
            unsigned int posy = rank / npx; 
            unsigned int offsx = posx * ndx; 
            unsigned int offsy = posy * ndy; 

            std::vector<float> myArray(ndx * ndy);

            adios2::ADIOS adios(comm);
            adios2::IO &io = adios.DeclareIO("writer");
            io.SetEngine("InSituMPI");

            adios2::Variable<float> &varArray = io.DefineVariable<float>(
                    "myArray", {gndx, gndy},
                    {offsx, offsy}, {ndx, ndy},
                    adios2::ConstantDims);

            adios2::Engine &writer =
                io.Open(streamName, adios2::Mode::Write, comm);

            for (int step = 0; step < steps; ++step)
            {
                int idx = 0;
                for (int j = 0; j < ndy; ++j)
                {
                    for (int i = 0; i < ndx; ++i)
                    {
                        myArray[idx] = rank + (step / 100.0f);
                        ++idx;
                    }
                }
                writer.BeginStep(adios2::StepMode::Append);
                writer.PutDeferred<float>(varArray, myArray.data());
                writer.EndStep();
                std::this_thread::sleep_for(
                        std::chrono::milliseconds(sleeptime));
            }
            writer.Close();
        }

        int MainReaders(MPI_Comm comm, unsigned int npx, unsigned int npy, unsigned int sleeptime)
        {
            int rank, nproc;
            MPI_Comm_rank(comm, &rank);
            MPI_Comm_size(comm, &nproc);
            if (!rank) 
            {
                std::cout << "There are " << nproc << "Readers" << std::endl;
            }

            adios2::ADIOS adios(comm);
            adios2::IO &io = adios.DeclareIO("reader");
            io.SetEngine("InSituMPI");
            adios2::Engine &reader =
                io.Open(streamName, adios2::Mode::Read, comm);

            unsigned int posx = rank % npx;
            unsigned int posy = rank / npx;
            int step = 0;
            adios2::Variable<float> *vMyArray = nullptr;
            std::vector<float> myArray;

            while (true)
            {
                adios2::StepStatus status =
                    reader.BeginStep(adios2::StepMode::NextAvailable, 60.0f);
                if (status != adios2::StepStatus::OK)
                {
                    break;
                }

                vMyArray = io.InquireVariable<float>("myArray");
                if (vMyArray == nullptr)
                {
                    throw std::ios_base::failure("Missing 'myArray' variable.");
                }


                // 2D decomposition of global array reading
                size_t gndx = vMyArray->m_Shape[0];
                size_t gndy = vMyArray->m_Shape[1];
                size_t ndx = gndx / npx;
                size_t ndy = gndy / npy;
                size_t offsx = ndx * posx;
                size_t offsy = ndy * posy;
                if (posx == npx - 1)
                {
                    // right-most processes need to read all the rest of rows
                    ndx = gndx - ndx * (npx - 1);
                }

                if (posy == npy - 1)
                {
                    // bottom processes need to read all the rest of columns
                    ndy = gndy - ndy * (npy - 1);
                }

                adios2::Dims count({ndx,ndy});
                adios2::Dims start({offsx,offsy});

                vMyArray->SetSelection({start, count});
                size_t elementsSize = count[0] * count[1];
                myArray.resize(elementsSize);

                reader.GetDeferred(*vMyArray, myArray.data());
                reader.EndStep();
                //checkData(myArray.data(), count, start, rank, step);
                ++step;
            }
            reader.Close();
        }

};

TEST_P(TestInSituMPIWriteRead, single)
{
    RunParams p = GetParam();
    std::cout << "test " 
        << p.npx_w << "x" << p.npy_w << " writers "
        << p.npx_r << "x" << p.npy_r << " readers "
        << std::endl;

    int nwriters = p.npx_w * p.npy_w;
    int nreaders = p.npx_r * p.npy_r;
    if (nwriters+nreaders > numprocs)
    {
        if (!wrank)
        {
        std::cout << "skip test: writers+readers > available processors "  
            << std::endl;
        }
        return;
    }

    
    int rank;
    MPI_Comm comm;

    unsigned int color;
    if (wrank < nwriters)
    {
        color = 0; // writers
    }
    else if (wrank < nwriters+nreaders)
    {
        color = 1; // readers
    }
    else
    {
        color = 2; // not participating in test
    }
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &comm);
    MPI_Comm_rank(comm, &rank);

    if (color == 0)
    {
        std::cout << "Process wrank " << wrank << " rank " << rank 
        << " calls MainWriters "
        << std::endl;
        MainWriters(comm, p.npx_w, p.npy_w, 10,1);
    }
    else if (color == 1)
    {
        std::cout << "Process wrank " << wrank << " rank " << rank 
        << " calls MainReaders "
        << std::endl;
        MainReaders(comm, p.npx_r, p.npy_r, 1);
    }
    std::cout << "Process wrank " << wrank << " rank " << rank 
        << " enters MPI barrier..."
        << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);
}

INSTANTIATE_TEST_CASE_P(NxM, TestInSituMPIWriteRead, ::testing::ValuesIn(CreateRunParams())); 
//INSTANTIATE_TEST_CASE_P(Writers, TestInSituMPIWriteRead, ::testing::Range(1,numprocs-1)); 
//INSTANTIATE_TEST_CASE_P(Writers, TestInSituMPIWriteRead, ::testing::Values(1,2)); 


//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    ::testing::InitGoogleTest(&argc, argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    //char *end;
    //numprocs = std::strtoll(argv[1], &end, 10);
    std::cout << "numprocs = " << numprocs << std::endl;

    /*
    range.resize(numprocs-1);
    std::iota(range.begin(), range.end(), 1);
    std::cout << "range = " ;
    for (const auto i : range)
    {
        std::cout << i << " ";
    }
    std::cout << std::endl;
    */

    int result = RUN_ALL_TESTS();

    MPI_Finalize();
    return result;
}
