/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InlineWriter.cpp
 * Inline engine from which any engine can be built.
 *
 *  Created on: Jan 04, 2018
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "InlineWriter.h"
#include "InlineWriter.tcc"

#include "adios2/helper/adiosFunctions.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

InlineWriter::InlineWriter(IO &io, const std::string &name, const Mode mode,
                               MPI_Comm mpiComm)
: Engine("InlineWriter", io, name, mode, mpiComm)
{
    m_EndMessage = " in call to InlineWriter " + m_Name + " Open\n";
    MPI_Comm_rank(mpiComm, &m_WriterRank);
    Init();
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Writer " << m_WriterRank << " Open(" << m_Name
                  << ")." << std::endl;
    }
}

StepStatus InlineWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    m_CurrentStep++; // 0 is the first step
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Writer " << m_WriterRank
                  << "   BeginStep() new step " << m_CurrentStep << "\n";
    }
    return StepStatus::OK;
}

size_t InlineWriter::CurrentStep() const
{
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Writer " << m_WriterRank
                  << "   CurrentStep() returns " << m_CurrentStep << "\n";
    }
    return m_CurrentStep;
}

/* PutDeferred = PutSync, so nothing to be done in PerformPuts */
void InlineWriter::PerformPuts()
{
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Writer " << m_WriterRank
                  << "     PerformPuts()\n";
    }
    m_NeedPerformPuts = false;
}

void InlineWriter::EndStep()
{
    if (m_NeedPerformPuts)
    {
        PerformPuts();
    }
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Writer " << m_WriterRank << "   EndStep()\n";
    }
}
void InlineWriter::Flush(const int transportIndex)
{
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Writer " << m_WriterRank << "   Flush()\n";
    }
}

// PRIVATE

#define declare_type(T)                                                        \
    void InlineWriter::DoPutSync(Variable<T> &variable, const T *data)       \
    {                                                                          \
        PutSyncCommon(variable, variable.SetBlockInfo(data, CurrentStep()));   \
        variable.m_BlocksInfo.clear();                                         \
    }                                                                          \
    void InlineWriter::DoPutDeferred(Variable<T> &variable, const T *data)   \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void InlineWriter::Init()
{
    InitParameters();
    InitTransports();
}

void InlineWriter::InitParameters()
{
    for (const auto &pair : m_IO.m_Parameters)
    {
        std::string key(pair.first);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        std::string value(pair.second);
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);

        if (key == "verbose")
        {
            m_Verbosity = std::stoi(value);
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
}

void InlineWriter::InitTransports()
{
    // Nothing to process from m_IO.m_TransportsParameters
}

void InlineWriter::DoClose(const int transportIndex)
{
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Writer " << m_WriterRank << " Close(" << m_Name
                  << ")\n";
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
