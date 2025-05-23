/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * A Use Case for reading metadata by one process only, then distribute that
 * among other processes/applications, which can "open" the dataset faster
 * by processing metadata from memory.
 *
 * Data is still read from disk by each process/application.
 *
 * This is an MPI application but every process acts as a separate entity as far as
 * reading with ADIOS goes.
 *
 * Created on: Aug 20, 2024
 *      Author: pnorbert
 */

#include <algorithm> // std::transform
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric> //std::accumulate
#include <thread>  // sleep_for
#include <vector>

#include <adios2.h>

#include <mpi.h>

typedef std::chrono::duration<double> Seconds;
typedef std::chrono::time_point<std::chrono::steady_clock,
                                std::chrono::duration<double, std::chrono::steady_clock::period>>
    TimePoint;

inline TimePoint Now() { return std::chrono::steady_clock::now(); }

struct VarInfo
{
    std::string varName;
    std::string type;
    adios2::Dims shape;
    adios2::ShapeID shapeID;
    size_t nSteps;
    std::vector<char> data;
    VarInfo(const std::string &name, const std::string &type, const adios2::Dims shape,
            const adios2::ShapeID shapeID, const size_t nsteps)
    : varName(name), type(type), shape(shape), shapeID(shapeID), nSteps(nsteps){};
};

std::string DimsToString(adios2::Dims &dims)
{
    std::string s = "";
    for (size_t i = 0; i < dims.size(); i++)
    {
        if (i > 0)
        {
            s += "x";
        }
        s += std::to_string(dims[i]);
    }
    s += "";
    return s;
}

size_t GetTotalSize(adios2::Dims &dimensions, size_t elementSize = 1)
{
    return std::accumulate(dimensions.begin(), dimensions.end(), elementSize,
                           std::multiplies<size_t>());
}

template <class T>
void ReadVariable(int rank, const std::string &name, const std::string &type,
                  adios2::Engine &reader, adios2::IO &io, std::vector<VarInfo> &varinfos)
{
    adios2::Variable<T> variable = io.InquireVariable<T>(name);
    varinfos.push_back(VarInfo(name, type, variable.Shape(), variable.ShapeID(), 1));
    auto vit = varinfos.rbegin();
    vit->nSteps = variable.Steps();
    if (vit->shapeID == adios2::ShapeID::GlobalArray)
    {
        size_t n = vit->nSteps * GetTotalSize(vit->shape, sizeof(T));
        vit->data.resize(n);
        adios2::Dims start(vit->shape.size());
        variable.SetSelection({start, vit->shape});
        variable.SetStepSelection({0, vit->nSteps});
        T *dataptr = reinterpret_cast<T *>(vit->data.data());
        reader.Get(variable, dataptr);
    }
    else if (vit->shapeID == adios2::ShapeID::GlobalValue)
    {
        size_t n = vit->nSteps * sizeof(T);
        vit->data.resize(n);
        variable.SetStepSelection({0, vit->nSteps});
        T *dataptr = reinterpret_cast<T *>(vit->data.data());
        reader.Get(variable, dataptr);
    }
}

std::vector<VarInfo> ReadFileContent(int rank, adios2::Engine &reader, adios2::IO &io)
{
    std::map<std::string, adios2::Params> varNameList = io.AvailableVariables();
    std::vector<VarInfo> varinfos;
    for (auto &var : varNameList)
    {
        const std::string &name(var.first);
        auto it = var.second.find("Type");
        const std::string &type = it->second;
        if (type == "struct")
        {
            // not supported
        }
#define declare_template_instantiation(T)                                                          \
    else if (type == adios2::GetType<T>())                                                         \
    {                                                                                              \
        ReadVariable<T>(rank, name, type, reader, io, varinfos);                                   \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }

    reader.PerformGets();
    return varinfos;
}

void ProcessFile(int rank, adios2::Engine &reader, adios2::IO &io, Seconds opentime)
{
    auto now = Now();
    std::vector<VarInfo> varinfos = ReadFileContent(rank, reader, io);
    Seconds readtime = Now() - now;

    std::cout << "File info on rank " << rank << ":" << std::endl;
    std::cout << "  Open time:   " << opentime.count() << "s" << std::endl;
    std::cout << "  Read time:   " << readtime.count() << "s" << std::endl;
    std::cout << "  Steps in file:   " << reader.Steps() << std::endl;
    std::cout << "  Total number of variables = " << varinfos.size() << std::endl;
    for (auto &vi : varinfos)
    {
        std::cout << "       Name: " << vi.varName << " dimensions = " << DimsToString(vi.shape)
                  << " steps = " << vi.nSteps << " size = " << vi.data.size() << " bytes"
                  << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " BP-file" << std::endl;
        return -1;
    }
    std::string fname = argv[1];

    int rank = 0, nproc = 1;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    /* Each process is acting as a serial program */
    adios2::ADIOS adios; // independent ADIOS object for each single process

    adios2::IO io = adios.DeclareIO("Input");
    char *fileMetadata;
    size_t fileMetadataSize;

    if (!rank)
    {
        std::cout << "First process opens file " << fname << std::endl;
        adios2::Engine reader;
        auto now = Now();
        reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
        Seconds opentime = Now() - now;
        reader.GetMetadata(&fileMetadata, &fileMetadataSize);
        std::cout << "Serialized metadata size = " << fileMetadataSize << std::endl;
        ProcessFile(rank, reader, io, opentime);
        reader.Close();
        std::cout << "\n=====  End of first process file processing =====\n" << std::endl;
    }

    /* Send metadata to all processes via MPI
     * (Note limitation to 2GB due MPI int)
     */
    MPI_Bcast(&fileMetadataSize, 1, MPI_INT64_T, 0, MPI_COMM_WORLD);
    if (fileMetadataSize > (size_t)std::numeric_limits<std::int32_t>::max())
    {
        if (!rank)
            std::cout << "ERROR: metadata size is >2GB, not supported by "
                         "MPI_BCast"
                      << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank)
    {
        fileMetadata = (char *)malloc(fileMetadataSize);
    }
    int mdsize = (int)fileMetadataSize;
    MPI_Bcast(fileMetadata, mdsize, MPI_CHAR, 0, MPI_COMM_WORLD);

    /* "Open" data by passing metadata to the adios engine */
    auto now = Now();
    adios2::Engine reader = io.Open(fname, fileMetadata, fileMetadataSize);
    Seconds opentime = Now() - now;
    free(fileMetadata);

    /* Process file in a sequentialized order only for pretty printing */
    MPI_Status status;
    int token = 0;
    if (rank > 0)
    {
        MPI_Recv(&token, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);
    }

    ProcessFile(rank, reader, io, opentime);

    if (rank < nproc - 1)
    {
        std::chrono::milliseconds timespan(100);
        std::this_thread::sleep_for(timespan);
        MPI_Send(&token, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
    }

    // Called once: indicate that we are done with this output for the run
    reader.Close();

    MPI_Finalize();
    return 0;
}
