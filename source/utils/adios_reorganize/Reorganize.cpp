/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Reorganize.cpp
 *
 *  Created on: Mar 7, 2018
 *      Author: Norbert Podhorszki, pnorbert@ornl.gov
 *
 * Reorganize global arrays
   Assumptions:
     - one output step fits into the memory of the reorganizer.
       Actually, this means, even more memory is needed than the size of output.
       We need to read each variable while also buffering all of them for
 output.
     - output steps contain the same variable set (no changes in variables)
     - attributes are the same for all steps (will write only once here)
 */

#include "Reorganize.h"

#include <assert.h>
#include <iomanip>
#include <iostream>
#include <string>

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/helper/adiosString.h"

// C headers
#include <cerrno>
#include <cstdlib>

namespace adios2
{
namespace utils
{

Reorganize::Reorganize(int argc, char *argv[])
: Utils("adios_reorganize", argc, argv)
{
    MPI_Comm_split(MPI_COMM_WORLD, m_MPISplitColor, rank, &comm);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &numproc);

    if (argc < 5)
    {
        PrintUsage();
        throw std::invalid_argument(
            "ERROR: Not enough arguments. At least 6 are required\n");
    }
    infilename = std::string(argv[1]);
    outfilename = std::string(argv[2]);
    rmethodname = std::string(argv[3]);
    rmethodparam_str = std::string(argv[4]);
    wmethodname = std::string(argv[5]);
    wmethodparam_str = std::string(argv[6]);

    int nd = 0;
    int j = 7;
    char *end;
    while (argc > j && j < 13)
    { // get max 6 dimensions
        errno = 0;
        decomp_values[nd] = std::strtol(argv[j], &end, 10);
        if (errno || (end != 0 && *end != '\0'))
        {
            std::string errmsg(
                "ERROR: Invalid decomposition number in argument " +
                std::to_string(j) + ": '" + std::string(argv[j]) + "'\n");
            PrintUsage();
            throw std::invalid_argument(errmsg);
        }
        nd++;
        j++;
    }

    if (argc > j)
    {
        throw std::invalid_argument(
            "ERROR: Up to 6 decomposition arguments are supported\n");
    }

    int prod = 1;
    for (int i = 0; i < nd; i++)
    {
        prod *= decomp_values[i];
    }

    if (prod > numproc)
    {
        print0("ERROR: Product of decomposition numbers %d > number of "
               "processes %d\n",
               prod, numproc);
        std::string errmsg("ERROR: The product of decomposition numbers " +
                           std::to_string(prod) + " > number of processes " +
                           std::to_string(numproc) + "\n");
        PrintUsage();
        throw std::invalid_argument(errmsg);
    }
}

void Reorganize::Run()
{
    ParseArguments();
    ProcessParameters();
    int retval = 0;

    print0("Input stream            = ", infilename);
    print0("Output stream           = ", outfilename);
    print0("Read method             = ", rmethodname);
    print0("Read method parameters  = ", rmethodparam_str);
    print0("Write method            = ", wmethodname);
    print0("Write method parameters = ", wmethodparam_str);

#ifdef ADIOS2_HAVE_MPI
    core::ADIOS adios(comm, true, "C++");
#else
    core::ADIOS adios(true, "C++");
#endif
    core::IO &io = adios.DeclareIO("group");

    print0("Waiting to open stream ", infilename, "...");

    io.SetEngine(rmethodname);
    io.SetParameters(rmethodparams);
    core::Engine &rStream = io.Open(infilename, adios2::Mode::Read);
    // rStream.FixedSchedule();

    io.SetEngine(wmethodname);
    io.SetParameters(wmethodparams);
    core::Engine &wStream = io.Open(outfilename, adios2::Mode::Write);

    int steps = 0;
    int curr_step = -1;
    while (true)
    {
        adios2::StepStatus status =
            rStream.BeginStep(adios2::StepMode::NextAvailable);
        if (status != adios2::StepStatus::OK)
        {
            break;
        }

        steps++; // start counting from 1

        if (rStream.CurrentStep() != curr_step + 1)
        {
            // we missed some steps
            std::cout << "rank " << rank << " WARNING: steps " << curr_step
                      << ".." << rStream.CurrentStep() - 1
                      << "were missed when advancing." << std::endl;
        }

        curr_step = static_cast<int>(rStream.CurrentStep());
        const core::DataMap &variables = io.GetVariablesDataMap();
        const core::DataMap &attributes = io.GetAttributesDataMap();

        print0("File info:");
        print0("  current step:   ", curr_step);
        print0("  # of variables: ", variables.size());
        print0("  # of attributes: ", attributes.size());

        retval = ProcessMetadata(rStream, io, variables, attributes, steps);
        if (retval)
            break;

        retval = ReadWrite(rStream, wStream, io, variables, steps);
        if (retval)
            break;

        CleanUpStep(io);
    }

    rStream.Close();
    wStream.Close();
    print0("Bye after processing ", steps, " steps");
}

// PRIVATE
template <typename Arg, typename... Args>
void Reorganize::osprint0(std::ostream &out, Arg &&arg, Args &&... args)
{
    if (!rank)
    {
        out << std::forward<Arg>(arg);
        using expander = int[];
        (void)expander{0, (void(out << std::forward<Args>(args)), 0)...};
        std::cout << std::endl;
    }
}

template <typename Arg, typename... Args>
void Reorganize::print0(Arg &&arg, Args &&... args)
{
    if (!rank)
    {
        std::cout << std::forward<Arg>(arg);
        using expander = int[];
        (void)expander{0, (void(std::cout << std::forward<Args>(args)), 0)...};
        std::cout << std::endl;
    }
}

Params Reorganize::parseParams(const std::string &param_str)
{
    std::istringstream ss(param_str);
    std::vector<std::string> kvs;
    std::string kv;

    while (std::getline(ss, kv, ','))
    {
        kvs.push_back(kv);
    }

    return helper::BuildParametersMap(kvs, true);
}

void Reorganize::ParseArguments()
{
    rmethodparams = parseParams(rmethodparam_str);
    wmethodparams = parseParams(wmethodparam_str);
}

void Reorganize::ProcessParameters() const {}

void Reorganize::PrintUsage() const noexcept
{
    std::cout
        << "Usage: adios_reorganize input output rmethod \"params\" wmethod "
           "\"params\" "
           "<decomposition>\n"
           "    input   Input stream path\n"
           "    output  Output file path\n"
           "    rmethod ADIOS method to read with\n"
           "            Supported read methods: BPFile, HDF5, SST, DataMan, "
           "InSituMPI\n"
           "    params  Read method parameters (in quotes; comma-separated "
           "list)\n"
           "    wmethod ADIOS method to write with\n"
           "    params  Write method parameters (in quotes; comma-separated "
           "list)\n"
           "    <decomposition>    list of numbers e.g. 32 8 4\n"
           "            Decomposition values in each dimension of an array\n"
           "            The product of these number must be less then the "
           "number\n"
           "            of processes. Processes whose rank is higher than the\n"
           "            product, will not write anything.\n"
           "               Arrays with less dimensions than the number of "
           "values,\n"
           "            will be decomposed with using the appropriate number "
           "of\n"
           "            values."
        << std::endl;
}

void Reorganize::PrintExamples() const noexcept {}

void Reorganize::SetParameters(const std::string argument, const bool isLong) {}

std::vector<VarInfo> varinfo;

// cleanup all info from previous step except
// do
//   remove all variable and attribute definitions from output group
//   free all varinfo (will be inquired again at next step)
//   free read buffer (required size may change at next step)
// do NOT
//   destroy group
//
void Reorganize::CleanUpStep(core::IO &io)
{
    for (auto &vi : varinfo)
    {
        if (vi.readbuf != nullptr)
        {
            free(vi.readbuf);
        }
    }
    varinfo.clear();
    // io.RemoveAllVariables();
    // io.RemoveAllAttributes();
}

template <typename T>
std::string Reorganize::VectorToString(const T &v)
{
    std::string s;
    for (const auto e : v)
    {
        s += std::to_string(e) + ", ";
    }
    s.pop_back();
    s.pop_back();
    return s;
}

size_t
Reorganize::Decompose(int numproc, int rank, VarInfo &vi,
                      const int *np // number of processes in each dimension
                      )
{
    size_t writesize = 0;
    if (vi.v == nullptr)
    {
        return writesize;
    }

    size_t ndim = vi.v->m_Shape.size();
    if (ndim == 0)
    {
        // scalars -> rank 0 writes them
        if (rank == 0)
            writesize = 1;
        else
            writesize = 0;
        return writesize;
    }

    /* calculate this process' position in the n-dim space
    0 1 2
    3 4 5
    6 7 8

    for 1D:
    posx = rank/1             ! 1st dim: 0, 1, 2...,rank-1 are in the same X
    position

    for 2D:
    posx = mod(rank, npx)     ! 1st dim: 0, npx, 2npx... are in the same X
    position
    posy = rank/(npx)         ! 2nd dim: npx processes belong into one dim

    for 3D:
    posx = mod(rank, npx)     ! 1st dim: 0, npx, 2npx... are in the same X
    position
    posy = mod(rank/npx, npy) ! 2nd dim: (0, npx-1) have the same dim (so divide
    with npx first)
    posz = rank/(npx*npy)     ! 3rd dim: npx*npy processes belong into one dim
    */
    int nps = 1;
    std::vector<int> pos(ndim); // rank's position in each dimensions
    vi.start.reserve(ndim);
    vi.count.reserve(ndim);

    size_t i = 0;
    for (i = 0; i < ndim - 1; i++)
    {
        pos[i] = (rank / nps) % np[i];
        nps *= np[i];
    }
    pos[i] = rank / nps;

    std::string ints = VectorToString(pos);
    if (pos[ndim - 1] >= np[ndim - 1])
    {
        std::cout << "rank " << rank << ": position in " << ndim
                  << "-D decomposition = " << ints
                  << " ---> Out of bound process" << std::endl;
    }
    else
    {
        std::cout << "rank " << rank << ": position in " << ndim
                  << "-D decomposition = " << ints << std::endl;
    }

    /* Decompose each dimension according to the position */
    writesize = 1;
    for (i = 0; i < ndim; i++)
    {
        size_t start, count;
        if (pos[ndim - 1] >= np[ndim - 1])
        {
            // this process gets nothing to read
            count = 0;
            start = 0;
        }
        else
        {
            count = vi.v->m_Shape[i] / np[i];
            start = count * pos[i];
            if (pos[i] == np[i] - 1)
            {
                // last one in the dimension may need to read more than the rest
                count = vi.v->m_Shape[i] - count * (np[i] - 1);
            }
        }
        vi.start.push_back(start);
        vi.count.push_back(count);
        writesize *= count;
    }
    ints = VectorToString(vi.count);
    std::cout << "rank " << rank << ": ldims in " << ndim << "-D space = {"
              << ints << "}" << std::endl;
    ints = VectorToString(vi.start);
    std::cout << "rank " << rank << ": offsets in " << ndim << "-D space = {"
              << ints << "}" << std::endl;
    return writesize;
}

int Reorganize::ProcessMetadata(core::Engine &rStream, core::IO &io,
                                const core::DataMap &variables,
                                const core::DataMap &attributes, int step)
{
    int retval = 0;

    varinfo.resize(variables.size());
    write_total = 0;
    largest_block = 0;

    // Decompose each variable and calculate output buffer size
    int varidx = 0;
    for (const auto &variablePair : variables)
    {
        const std::string &name(variablePair.first);
        const std::string &type(variablePair.second.first);
        core::VariableBase *variable = nullptr;
        print0("Get info on variable ", varidx, ": ", name);

        if (type == "compound")
        {
            // not supported
        }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        variable = io.InquireVariable<T>(variablePair.first);                  \
    }
        ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

        varinfo[varidx].v = variable;

        if (variable == nullptr)
        {
            std::cerr << "rank " << rank << ": ERROR: Variable " << name
                      << " inquiry failed" << std::endl;
            return 1;
        }

        // print variable type and dimensions
        if (!rank)
        {
            std::cout << "    " << type << " " << name;
            if (variable->m_Shape.size() > 0)
            {
                std::cout << "[" << variable->m_Shape[0];
                for (int j = 1; j < variable->m_Shape.size(); j++)
                    std::cout << ", " << variable->m_Shape[j];
                std::cout << "]" << std::endl;
            }
            else
            {
                print0("\tscalar\n");
            }
        }

        // determine subset we will write
        size_t sum_count =
            Decompose(numproc, rank, varinfo[varidx], decomp_values);
        varinfo[varidx].writesize = sum_count * variable->m_ElementSize;

        if (varinfo[varidx].writesize != 0)
        {
            write_total += varinfo[varidx].writesize;
            if (largest_block < varinfo[varidx].writesize)
                largest_block = varinfo[varidx].writesize;
        }
        ++varidx;
    }

    // determine output buffer size
    size_t bufsize =
        write_total + variables.size() * 200 + attributes.size() * 32 + 1024;
    if (bufsize > max_write_buffer_size)
    {
        std::cerr << "ERROR: rank " << rank
                  << ": write buffer size needs to hold about " << bufsize
                  << "bytes but max is set to " << max_write_buffer_size
                  << std::endl;
        return 1;
    }

    if (bufsize > max_read_buffer_size)
    {
        std::cerr << "ERROR: rank " << rank
                  << ": read buffer size needs to hold at least " << bufsize
                  << "bytes but max is set to " << max_read_buffer_size
                  << std::endl;
        return 1;
    }
    return retval;
}

int Reorganize::ReadWrite(core::Engine &rStream, core::Engine &wStream,
                          core::IO &io, const core::DataMap &variables,
                          int step)
{
    int retval = 0;

    size_t nvars = variables.size();
    if (nvars != varinfo.size())
    {
        std::cerr
            << "ERROR rank " << rank << ": Invalid program state, number "
                                        "of variables ("
            << nvars
            << ") to read does not match the number of processed variables ("
            << varinfo.size() << ")" << std::endl;
    }

    /*
     * Read all variables into memory
     */
    for (size_t varidx = 0; varidx < nvars; ++varidx)
    {
        const std::string &name = varinfo[varidx].v->m_Name;
        assert(varinfo[varidx].readbuf == nullptr);
        if (varinfo[varidx].writesize != 0)
        {
            // read variable subset
            std::cout << "rank " << rank << ": Read variable " << name
                      << std::endl;
            const std::string &type = variables.at(name).first;
            if (type == "compound")
            {
                // not supported
            }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        varinfo[varidx].readbuf = calloc(1, varinfo[varidx].writesize);        \
        if (varinfo[varidx].count.size() == 0)                                 \
        {                                                                      \
            rStream.Get<T>(name,                                               \
                           reinterpret_cast<T *>(varinfo[varidx].readbuf),     \
                           adios2::Mode::Sync);                                \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            varinfo[varidx].v->SetSelection(                                   \
                {varinfo[varidx].start, varinfo[varidx].count});               \
            rStream.Get<T>(name,                                               \
                           reinterpret_cast<T *>(varinfo[varidx].readbuf));    \
        }                                                                      \
    }
            ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
        }
    }
    rStream.EndStep(); // read in data into allocated pointers

    /*
     * Write all variables
     */
    wStream.BeginStep();
    for (size_t varidx = 0; varidx < nvars; ++varidx)
    {
        const std::string &name = varinfo[varidx].v->m_Name;
        if (varinfo[varidx].writesize != 0)
        {
            // Write variable subset
            std::cout << "rank " << rank << ": Write variable " << name
                      << std::endl;
            const std::string &type = variables.at(name).first;
            if (type == "compound")
            {
                // not supported
            }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        if (varinfo[varidx].count.size() == 0)                                 \
        {                                                                      \
            wStream.Put<T>(name,                                               \
                           reinterpret_cast<T *>(varinfo[varidx].readbuf),     \
                           adios2::Mode::Sync);                                \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            varinfo[varidx].v->SetSelection(                                   \
                {varinfo[varidx].start, varinfo[varidx].count});               \
            wStream.Put<T>(name,                                               \
                           reinterpret_cast<T *>(varinfo[varidx].readbuf));    \
        }                                                                      \
    }
            ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
        }
    }
    wStream.EndStep(); // write output buffer to file
    return retval;
}

} // end namespace utils
} // end namespace adios2
