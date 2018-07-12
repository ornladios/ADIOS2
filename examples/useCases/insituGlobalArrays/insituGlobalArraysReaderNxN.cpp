/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * A Use Case for In Situ visulization frameworks (Conduit, SENSEI)
 *
 * Read in the variables that the Writer wrote.
 * Every process should read only what the corresponding Writer wrote
 * This is an N to N case
 *
 * Created on: Jul 11, 2017
 *      Author: pnorbert
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <thread> // sleep_for


#include <adios2.h>
#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

typedef struct
{
    //core::VariableBase *v = nullptr;
    std::string varName;
    std::string type;
    adios2::Dims shape;
    adios2::Dims start;
    adios2::Dims count;
    //void *readbuf = nullptr; // read in buffer
} VarInfo;


std::vector<VarInfo> ProcessMetadata(int rank,
        const adios2::Engine &reader, const adios2::IO &io, 
        const std::map<std::string, adios2::Params> &varNameList)
{
    std::vector<VarInfo> varinfos; 
    for (auto &var : varNameList)
    {
        const std::string &name(var.first);
        auto it = var.second.find("Type");
        const std::string &type = it->second;
        it = var.second.find("Shape");
        const std::string &shape = it->second;
        if (!rank)
        {
        std::cout << "    Variable '" << name << "' type " << type   
            << " dimensions = " << shape << std::endl;
        }
/*        if (type == "compound")
        {
            // not supported
        }
#define declare_template_instantiation(T)                                      \
        else if (type == helper::GetType<T>())                                     \
        {                                                                          \
            auto variable = io.InquireVariable<T>(variablePair.first);                  \
        }
        ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

    */
    }
    return varinfos;
}

void SerialPrintout(std::vector<VarInfo> &varinfos, int rank, int nproc)
{
    // Serialize printout
#ifdef ADIOS2_HAVE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
    int token = 0;
    MPI_Status st;
    if (rank > 0)
    {
        MPI_Recv(&token, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, &st);
    }
#endif

    std::cout << "    Rank " << rank << " variables:" << varinfos.size() << std::endl;
    for (auto &vi : varinfos)
    {
        std::cout << "       Name: " << vi.varName << std::endl;
    }

#ifdef ADIOS2_HAVE_MPI
    if (rank < nproc-1)
    {
        //std::chrono::milliseconds timespan(100);
        //std::this_thread::sleep_for(timespan);
        MPI_Send(&token, 1, MPI_INT, rank+1, 0, MPI_COMM_WORLD);
    }
#endif
    MPI_Barrier(MPI_COMM_WORLD);
}



int main(int argc, char *argv[])
{
    int rank = 0, nproc = 1;
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
#endif

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif

    try
    {
        adios2::IO io = adios.DeclareIO("Input");
        adios2::Engine reader = io.Open("output.bp", adios2::Mode::Read);

        while (true)
        {
            adios2::StepStatus status =
                reader.BeginStep(adios2::StepMode::NextAvailable, 0.0f);
            if (status != adios2::StepStatus::OK)
            {
                break;
            }

            std::map<std::string, adios2::Params> varNameList = io.AvailableVariables();
            const size_t nTotalVars = varNameList.size();
            if (!rank) 
            {
                std::cout << "File info:" << std::endl;
                std::cout << "  Current step:   " << reader.CurrentStep() << std::endl;
                std::cout << "  Total number of variables = " << nTotalVars << std::endl;
            }

            std::vector<VarInfo> varinfos = ProcessMetadata(rank, reader, io, varNameList);
            SerialPrintout(varinfos, rank, nproc);
            
            reader.EndStep();
        }

        // Called once: indicate that we are done with this output for the run
        reader.Close();
    }
    catch (std::invalid_argument &e)
    {
        if (rank == 0)
        {
            std::cout << "Invalid argument exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }
    catch (std::ios_base::failure &e)
    {
        if (rank == 0)
        {
            std::cout << "System exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }
    catch (std::exception &e)
    {
        if (rank == 0)
        {
            std::cout << "Exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return 0;
}
