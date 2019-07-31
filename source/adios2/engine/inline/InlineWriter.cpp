/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InlineWriter.cpp
 *
 *  Created on: Nov 16, 2018
 *      Author: Aron Helser aron.helser@kitware.com
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
                           helper::Comm comm)
: Engine("InlineWriter", io, name, mode, std::move(comm))
{
    m_EndMessage = " in call to InlineWriter " + m_Name + " Open\n";
    m_WriterRank = m_Comm.Rank();
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

    // Need to clear block info from previous step at this point.
    if (m_ReadVariables.empty())
    {
        return StepStatus::OK;
    }

    for (const std::string &name : m_ReadVariables)
    {
        const std::string type = m_IO.InquireVariableType(name);

        if (type == "compound")
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        Variable<T> &variable = FindVariable<T>(name, "in call to BeginStep"); \
        variable.m_BlocksInfo.clear();                                         \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    }

    m_ReadVariables.clear();

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
        std::cout << "Inline Writer " << m_WriterRank << "     PerformPuts()\n";
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
    void InlineWriter::DoPutSync(Variable<T> &variable, const T *data)         \
    {                                                                          \
        PutSyncCommon(variable, variable.SetBlockInfo(data, CurrentStep()));   \
        /*reader uses: variable.m_BlocksInfo.clear();*/                        \
    }                                                                          \
    void InlineWriter::DoPutDeferred(Variable<T> &variable, const T *data)     \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
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
