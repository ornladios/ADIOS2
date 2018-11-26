/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * StagingReader.cpp
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#include "StagingReader.h"
#include "StagingReader.tcc"

#include "adios2/helper/adiosFunctions.h" // CSVToVector
#include "adios2/toolkit/transport/file/FileFStream.h"

#include <iostream>

#include <zmq.h>

namespace adios2
{
namespace core
{
namespace engine
{

StagingReader::StagingReader(IO &io, const std::string &name, const Mode mode,
                             MPI_Comm mpiComm)
: Engine("StagingReader", io, name, mode, mpiComm),
  m_DataManSerializer(helper::IsRowMajor(io.m_HostLanguage), true,
                      helper::IsLittleEndian()),
  m_MetadataTransport(mpiComm, m_DebugMode),
  m_DataTransport(mpiComm, m_DebugMode)
{
    m_EndMessage = " in call to IO Open StagingReader " + m_Name + "\n";
    MPI_Comm_rank(mpiComm, &m_MpiRank);
    Init();
    if (m_Verbosity == 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " Open(" << m_Name
                  << ") in constructor." << std::endl;
    }
}

StagingReader::~StagingReader()
{
    /* m_Staging deconstructor does close and finalize */
    if (m_Verbosity == 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " deconstructor on "
                  << m_Name << "\n";
    }
}

StepStatus StagingReader::BeginStep(const StepMode stepMode,
                                    const float timeoutSeconds)
{

    // Receive aggregated metadata from writer master rank

    std::shared_ptr<std::vector<char>> buff = nullptr;

    if (m_MpiRank == 0)
    {
        while (buff == nullptr)
        {
            buff = m_MetadataTransport.Read(0);
        }
    }
    else
    {
        buff = std::make_shared<std::vector<char>>();
    }

    m_DataManSerializer.PutAggregatedMetadata(m_MPIComm, buff);

    if (m_CurrentStep == 0)
    {
        m_DataManSerializer.GetAttributes(m_IO);
    }

    auto m_MetaDataMap = m_DataManSerializer.GetMetaData();

    size_t maxStep = std::numeric_limits<size_t>::min();
    size_t minStep = std::numeric_limits<size_t>::max();

    for (auto &i : m_MetaDataMap)
    {
        if (i.first > maxStep)
        {
            maxStep = i.first;
        }
        if (i.first < minStep)
        {
            minStep = i.first;
        }
    }

    if (stepMode == StepMode::NextAvailable)
    {
        m_CurrentStep = minStep;
    }
    else if (stepMode == StepMode::LatestAvailable)
    {
        m_CurrentStep = maxStep;
    }
    else
    {
        throw(std::invalid_argument(
            "[StagingReader::BeginStep] Step mode is not supported!"));
    }

    std::shared_ptr<std::vector<format::DataManSerializer::DataManVar>> vars =
        nullptr;
    auto currentStepIt = m_MetaDataMap.find(m_CurrentStep);
    if (currentStepIt != m_MetaDataMap.end())
    {
        vars = currentStepIt->second;
    }

    if (vars != nullptr)
    {

        for (const auto &i : *vars)
        {
            if (i.step == m_CurrentStep)
            {
                if (i.type == "compound")
                {
                    throw("Compound type is not supported yet.");
                }
#define declare_type(T)                                                        \
    else if (i.type == helper::GetType<T>())                                   \
    {                                                                          \
        CheckIOVariable<T>(i.name, i.shape, i.start, i.count);                 \
    }
                ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
                else
                {
                    throw("Unknown type caught in "
                          "DataManReader::BeginStepSubscribe.");
                }
            }
        }
    }

    if (m_Verbosity == 5)
    {
        std::cout << "Staging Reader " << m_MpiRank
                  << "   BeginStep() new step " << m_CurrentStep << "\n";
    }

    // We should block until a new step arrives or reach the timeout

    // m_IO Variables and Attributes should be defined at this point
    // so that the application can inquire them and start getting data

    return StepStatus::OK;
}

void StagingReader::PerformGets()
{

    auto requests = m_DataManSerializer.GetDeferredRequest();
    for (const auto &i : *requests)
    {
        std::shared_ptr<std::vector<char>> reply =
            std::make_shared<std::vector<char>>();
        m_DataTransport.Request(i.second, reply, i.first);
        m_DataManSerializer.PutPack(reply);
    }

    auto varsVec = m_MetaDataMap.find(m_CurrentStep);
    if (varsVec == m_MetaDataMap.end())
    {
        return;
    }
    if (varsVec->second == nullptr)
    {
        return;
    }

    for (const auto &req : m_DeferredRequests)
    {
        if (req.type == "compound")
        {
            throw("Compound type is not supported yet.");
        }
#define declare_type(T)                                                        \
    else if (req.type == helper::GetType<T>())                                 \
    {                                                                          \
        m_DataManSerializer.GetVar(reinterpret_cast<T *>(req.data),            \
                                   req.variable, req.start, req.count,         \
                                   m_CurrentStep, req.start, req.count);       \
    }
        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
    }

    if (m_Verbosity == 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " PerformGets()\n";
    }
}

size_t StagingReader::CurrentStep() const { return m_CurrentStep; }

void StagingReader::EndStep()
{
    PerformGets();
    m_DataManSerializer.Erase(CurrentStep());
    if (m_Verbosity == 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " EndStep()\n";
    }
}

// PRIVATE

#define declare_type(T)                                                        \
    void StagingReader::DoGetSync(Variable<T> &variable, T *data)              \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void StagingReader::DoGetDeferred(Variable<T> &variable, T *data)          \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void StagingReader::Init()
{
    InitParameters();
    Handshake();
    InitTransports();
}

void StagingReader::InitParameters()
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

void StagingReader::InitTransports()
{
    if (m_MpiRank == 0)
    {
        Params params;
        params["IPAddress"] = m_WriterMasterIP;
        params["Port"] = m_WriterMasterMetadataPort;
        params["Library"] = "zmq";
        params["Name"] = m_Name;
        std::vector<Params> paramsVec;
        paramsVec.emplace_back(params);
        m_MetadataTransport.OpenTransports(paramsVec, Mode::Read, "subscribe",
                                           true);
    }
}

void StagingReader::Handshake()
{
    transport::FileFStream ipstream(m_MPIComm, m_DebugMode);
    ipstream.Open(".StagingHandshake", Mode::Read);
    auto size = ipstream.GetSize();
    std::vector<char> ip(size);
    ipstream.Read(ip.data(), size);
    ipstream.Close();
    m_WriterMasterIP = std::string(ip.begin(), ip.end());
}

void StagingReader::DoClose(const int transportIndex)
{
    if (m_Verbosity == 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " Close(" << m_Name
                  << ")\n";
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
