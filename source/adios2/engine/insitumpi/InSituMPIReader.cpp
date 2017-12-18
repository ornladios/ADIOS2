/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPIReader.cpp
 *
 *  Created on: Dec 18, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "InSituMPIReader.h"
#include "InSituMPIFunctions.h"
#include "InSituMPIReader.tcc"

#include <adios_error.h>

#include "adios2/helper/adiosFunctions.h" // CSVToVector

#include <iostream>

namespace adios2
{

InSituMPIReader::InSituMPIReader(IO &io, const std::string &name,
                                 const Mode mode, MPI_Comm mpiComm)
: Engine("InSituMPIReader", io, name, mode, mpiComm)
{
    m_EndMessage = " in call to IO Open InSituMPIReader " + m_Name + "\n";

    Init();

    m_WriterPeers = insitumpi::FindPeers(mpiComm, m_Name, false);
    MPI_Comm_rank(MPI_COMM_WORLD, &m_GlobalRank);
    MPI_Comm_size(MPI_COMM_WORLD, &m_GlobalNproc);
    MPI_Comm_rank(mpiComm, &m_ReaderRank);
    MPI_Comm_size(mpiComm, &m_ReaderNproc);
    m_WriterNproc = m_WriterPeers.size();
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank << " Open(" << m_Name
                  << "). Fixed schedule = " << (m_FixedSchedule ? "yes" : "no")
                  << ". #readers=" << m_ReaderNproc
                  << " #writers=" << m_WriterNproc
                  << " #appsize=" << m_GlobalNproc << std::endl;
    }
}

InSituMPIReader::~InSituMPIReader()
{
    if (m_Verbosity == 5)
    {
        std::cout << "Destroying InSituMPI Reader " << m_Name << "\n";
    }
}

StepStatus InSituMPIReader::BeginStep(const StepMode mode,
                                      const float timeoutSeconds)
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader BeginStep()\n";
    }
    return StepStatus::EndOfStream;
}

void InSituMPIReader::PerformGets()
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader PerformGets()\n";
    }
}

void InSituMPIReader::EndStep()
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader EndStep()\n";
    }
}

void InSituMPIReader::Close(const int transportIndex)
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader Close(" << m_Name << ")\n";
    }
}

// PRIVATE

#define declare_type(T)                                                        \
    void InSituMPIReader::DoGetSync(Variable<T> &variable, T *data)            \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void InSituMPIReader::DoGetDeferred(Variable<T> &variable, T *data)        \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }                                                                          \
    void InSituMPIReader::DoGetDeferred(Variable<T> &variable, T &data)        \
    {                                                                          \
        GetDeferredCommon(variable, &data);                                    \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void InSituMPIReader::Init()
{
    InitParameters();
    InitTransports();
}

void InSituMPIReader::InitParameters()
{
    auto itFixedSchedule = m_IO.m_Parameters.find("FixedSchedule");
    if (itFixedSchedule == m_IO.m_Parameters.end())
    {
        m_FixedSchedule = false;
    }
    else if (itFixedSchedule->second == "true")
    {
        m_FixedSchedule = true;
    }

    auto itVerbosity = m_IO.m_Parameters.find("verbose");
    if (itVerbosity != m_IO.m_Parameters.end())
    {
        m_Verbosity = std::stoi(itVerbosity->second);
        if (m_DebugMode)
        {
            if (m_Verbosity < 0 || m_Verbosity > 5)
                throw std::invalid_argument(
                    "ERROR: Method verbose argument must be an "
                    "integer in the range [0,5], in call to "
                    "Open or Engine constructor\n");
        }
    }
}

void InSituMPIReader::InitTransports()
{
    // Nothing to process from m_IO.m_TransportsParameters
}

} // end namespace adios2
