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

namespace adios2
{
namespace core
{
namespace engine
{

StagingReader::StagingReader(IO &io, const std::string &name, const Mode mode,
                             MPI_Comm mpiComm)
: Engine("StagingReader", io, name, mode, mpiComm),
  m_DataManSerializer(helper::IsRowMajor(io.m_HostLanguage),
                      helper::IsLittleEndian()),
  m_DataTransport(mpiComm, m_DebugMode),
  m_MetadataTransport(mpiComm, m_DebugMode)
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

StepStatus StagingReader::BeginStep(const StepMode mode,
                                    const float timeoutSeconds)
{
    // step info should be received from the writer side in BeginStep()
    // so this forced increase should not be here
    ++m_CurrentStep;

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

    auto metadata = m_DataManSerializer.GetMetaData();

    for (auto &i : metadata)
    {
        std::cout << "BeginStep" << i.first << std::endl;
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
    if (m_Verbosity == 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " PerformGets()\n";
    }
}

size_t StagingReader::CurrentStep() const { return m_CurrentStep; }

void StagingReader::EndStep()
{
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
