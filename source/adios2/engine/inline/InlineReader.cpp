/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InlineReader.cpp
 *
 *  Created on: Nov 16, 2018
 *      Author: Aron Helser aron.helser@kitware.com
 */

#include "InlineReader.h"
#include "InlineReader.tcc"

#include "adios2/helper/adiosFunctions.h" // CSVToVector

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

InlineReader::InlineReader(IO &io, const std::string &name, const Mode mode,
                           helper::Comm comm)
: Engine("InlineReader", io, name, mode, std::move(comm))
{
    m_EndMessage = " in call to IO Open InlineReader " + m_Name + "\n";
    m_ReaderRank = m_Comm.Rank();
    Init();
    Engine &writer = io.GetEngine(m_WriterID);
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Reader " << m_ReaderRank << " Open(" << m_Name
                  << ") in constructor, with writer: " << writer.m_Name
                  << std::endl;
    }
}

InlineReader::~InlineReader()
{
    /* m_Inline deconstructor does close and finalize */
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Reader " << m_ReaderRank << " deconstructor on "
                  << m_Name << "\n";
    }
}

StepStatus InlineReader::BeginStep(const StepMode mode,
                                   const float timeoutSeconds)
{
    // step info should be received from the writer side in BeginStep()
    // so this forced increase should not be here
    ++m_CurrentStep;

    if (m_Verbosity == 5)
    {
        std::cout << "Inline Reader " << m_ReaderRank
                  << "   BeginStep() new step " << m_CurrentStep << "\n";
    }

    // m_IO Variables and Attributes should be defined at this point
    // so that the application can inquire them and start getting data

    return StepStatus::OK;
}

void InlineReader::PerformGets()
{
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Reader " << m_ReaderRank << "     PerformGets()\n";
    }
    m_NeedPerformGets = false;
}

size_t InlineReader::CurrentStep() const { return m_CurrentStep; }

void InlineReader::EndStep()
{
    // EndStep should call PerformGets() if there are unserved GetDeferred()
    // requests
    if (m_NeedPerformGets)
    {
        PerformGets();
    }

    if (m_Verbosity == 5)
    {
        std::cout << "Inline Reader " << m_ReaderRank << "   EndStep()\n";
    }
}

// PRIVATE

#define declare_type(T)                                                        \
    void InlineReader::DoGetSync(Variable<T> &variable, T *data)               \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void InlineReader::DoGetDeferred(Variable<T> &variable, T *data)           \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }                                                                          \
    typename Variable<T>::Info *InlineReader::DoGetBlockSync(                  \
        Variable<T> &variable)                                                 \
    {                                                                          \
        return GetBlockSyncCommon(variable);                                   \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

// Design note: Returns a copy. Instead, could return a reference, then
// Engine::Get() would not need an Info parameter passed in - binding could
// retrieve the current Core Info object at a later time.
// See note on binding Engine::BlocksInfo
#define declare_type(T)                                                        \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    InlineReader::DoAllStepsBlocksInfo(const Variable<T> &variable) const      \
    {                                                                          \
        return std::map<size_t, std::vector<typename Variable<T>::Info>>();    \
    }                                                                          \
                                                                               \
    std::vector<typename Variable<T>::Info> InlineReader::DoBlocksInfo(        \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        return variable.m_BlocksInfo;                                          \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void InlineReader::Init()
{
    InitParameters();
    InitTransports();
}

void InlineReader::InitParameters()
{
    for (const auto &pair : m_IO.m_Parameters)
    {
        std::string key(pair.first);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        std::string value(pair.second);
        // std::transform(value.begin(), value.end(), value.begin(), ::tolower);

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
        else if (key == "writerid")
        {
            m_WriterID = value;
            if (m_Verbosity == 5)
            {
                std::cout << "Inline Reader " << m_ReaderRank
                          << " Init() writerID " << m_WriterID << "\n";
            }
        }
    }
}

void InlineReader::InitTransports()
{
    // Nothing to process from m_IO.m_TransportsParameters
}

void InlineReader::DoClose(const int transportIndex)
{
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Reader " << m_ReaderRank << " Close(" << m_Name
                  << ")\n";
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
