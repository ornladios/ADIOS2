/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TransportMan.cpp
 *
 *  Created on: May 23, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "TransportMan.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <set>
/// \endcond

#include "adios2/helper/adiosFunctions.h" //CreateDirectory

/// transports
#ifndef _WIN32
#include "adios2/toolkit/transport/file/FilePOSIX.h"
#endif

#include "adios2/toolkit/transport/file/FileFStream.h"
#include "adios2/toolkit/transport/file/FileStdio.h"

namespace adios2
{
namespace transportman
{

TransportMan::TransportMan(MPI_Comm mpiComm, const bool debugMode)
: m_MPIComm(mpiComm), m_DebugMode(debugMode)
{
}

void TransportMan::OpenFiles(const std::vector<std::string> &fileNames,
                             const Mode openMode,
                             const std::vector<Params> &parametersVector,
                             const bool profile)
{

    for (size_t i = 0; i < fileNames.size(); ++i)
    {
        const Params &parameters = parametersVector[i];
        const std::string type(parameters.at("transport"));

        if (type == "File" || type == "file") // need to create directory
        {
            std::shared_ptr<Transport> file =
                OpenFileTransport(fileNames[i], openMode, parameters, profile);
            m_Transports.insert({i, file});
        }
    }
}

void TransportMan::OpenFileID(const std::string &name, const size_t id,
                              const Mode mode, const Params &parameters,
                              const bool profile)
{
    std::shared_ptr<Transport> file =
        OpenFileTransport(name, mode, parameters, profile);
    m_Transports.insert({id, file});
}

std::vector<std::string> TransportMan::GetFilesBaseNames(
    const std::string &baseName,
    const std::vector<Params> &parametersVector) const
{
    if (parametersVector.size() <= 1)
    {
        return {baseName};
    }

    std::map<std::string, std::set<std::string>> typeTransportNames;
    std::vector<std::string> baseNames;
    baseNames.reserve(parametersVector.size());

    for (const auto &parameters : parametersVector)
    {
        // Get transport name from user
        std::string name(baseName);
        SetParameterValue("Name", parameters, name); // if found in map

        const std::string type(parameters.at("transport"));

        if (m_DebugMode)
        {
            auto itType = typeTransportNames.find(type);
            // check if name exists for this transport type
            if (itType != typeTransportNames.end())
            {
                if (itType->second.count(name) == 1)
                {
                    throw std::invalid_argument(
                        "ERROR: two IO AddTransport of the same type can't "
                        "have the same name : " +
                        name + ", use Name=value parameter, in "
                               "call to Open");
                }
            }
        }
        typeTransportNames[type].insert(name);
        baseNames.push_back(name);
    }
    return baseNames;
}

std::vector<std::string> TransportMan::GetTransportsTypes() noexcept
{
    std::vector<std::string> types;
    types.reserve(m_Transports.size());

    for (const auto &transportPair : m_Transports)
    {
        const std::shared_ptr<Transport> &transport = transportPair.second;
        types.push_back(transport->m_Type + "_" + transport->m_Library);
    }
    return types;
}

std::vector<profiling::IOChrono *>
TransportMan::GetTransportsProfilers() noexcept
{
    std::vector<profiling::IOChrono *> profilers;
    profilers.reserve(m_Transports.size());

    for (const auto &transportPair : m_Transports)
    {
        const auto &transport = transportPair.second;
        profilers.push_back(&transport->m_Profiler);
    }
    return profilers;
}

void TransportMan::WriteFiles(const char *buffer, const size_t size,
                              const int transportIndex)
{
    if (transportIndex == -1)
    {
        for (auto &transportPair : m_Transports)
        {
            auto &transport = transportPair.second;
            if (transport->m_Type == "File")
            {
                // make this truly asynch?
                transport->Write(buffer, size);
            }
        }
    }
    else
    {
        auto itTransport = m_Transports.find(transportIndex);
        CheckFile(itTransport, ", in call to WriteFiles with index " +
                                   std::to_string(transportIndex));
        itTransport->second->Write(buffer, size);
    }
}

size_t TransportMan::GetFileSize(const size_t transportIndex) const
{
    auto itTransport = m_Transports.find(transportIndex);
    CheckFile(itTransport, ", in call to GetFileSize with index " +
                               std::to_string(transportIndex));
    return itTransport->second->GetSize();
}

void TransportMan::ReadFile(char *buffer, const size_t size, const size_t start,
                            const size_t transportIndex)
{
    auto itTransport = m_Transports.find(transportIndex);
    CheckFile(itTransport, ", in call to ReadFile with index " +
                               std::to_string(transportIndex));
    itTransport->second->Read(buffer, size, start);
}

void TransportMan::CloseFiles(const int transportIndex)
{
    if (transportIndex == -1)
    {
        for (auto &transportPair : m_Transports)
        {
            auto &transport = transportPair.second;

            if (transport->m_Type == "File")
            {
                transport->Close();
            }
        }
    }
    else
    {
        auto itTransport = m_Transports.find(transportIndex);
        CheckFile(itTransport, ", in call to CloseFiles with index " +
                                   std::to_string(transportIndex));
        itTransport->second->Close();
    }
}

bool TransportMan::AllTransportsClosed() const noexcept
{
    bool allClose = true;
    for (const auto &transportPair : m_Transports)
    {
        const auto &transport = transportPair.second;

        if (transport->m_IsOpen)
        {
            allClose = false;
            break;
        }
    }
    return allClose;
}

// PRIVATE
std::shared_ptr<Transport>
TransportMan::OpenFileTransport(const std::string &fileName,
                                const Mode openMode, const Params &parameters,
                                const bool profile)
{
    auto lf_SetFileTransport = [&](const std::string library,
                                   std::shared_ptr<Transport> &transport) {
        if (library == "stdio")
        {
            transport =
                std::make_shared<transport::FileStdio>(m_MPIComm, m_DebugMode);
        }
        else if (library == "fstream")
        {
            transport = std::make_shared<transport::FileFStream>(m_MPIComm,
                                                                 m_DebugMode);
        }
#ifndef _WIN32
        else if (library == "POSIX")
        {
            transport =
                std::make_shared<transport::FilePOSIX>(m_MPIComm, m_DebugMode);
        }
#endif
        else
        {
            if (m_DebugMode)
            {
                throw std::invalid_argument(
                    "ERROR: invalid IO AddTransport library " + library +
                    ", only POSIX, stdio, fstream are supported\n");
            }
        }
    };

    auto lf_GetLibrary = [](const std::string defaultLibrary,
                            const Params &parameters) -> std::string {

        std::string library(defaultLibrary);
        SetParameterValue("Library", parameters, library);
        return library;
    };

    auto lf_GetTimeUnits = [&](const std::string defaultTimeUnit,
                               const Params &parameters) -> TimeUnit {

        std::string profileUnits(defaultTimeUnit);
        SetParameterValue("ProfileUnits", parameters, profileUnits);
        return StringToTimeUnit(profileUnits, m_DebugMode);
    };

    // BODY OF FUNCTION starts here
    std::shared_ptr<Transport> transport;
    lf_SetFileTransport(lf_GetLibrary(DefaultFileLibrary, parameters),
                        transport);

    // Default or user ProfileUnits in parameters
    if (profile)
    {
        transport->InitProfiler(openMode,
                                lf_GetTimeUnits(DefaultTimeUnit, parameters));
    }

    // open
    transport->Open(fileName, openMode);
    return transport;
}

void TransportMan::CheckFile(
    std::unordered_map<size_t, std::shared_ptr<Transport>>::const_iterator
        itTransport,
    const std::string hint) const
{
    if (m_DebugMode)
    {
        if (itTransport == m_Transports.end())
        {
            throw std::invalid_argument("ERROR: invalid transport " + hint +
                                        "\n");
        }

        if (itTransport->second->m_Type != "File")
        {
            throw std::invalid_argument("ERROR: invalid type " +
                                        itTransport->second->m_Library +
                                        ", must be file " + hint + "\n");
        }
    }
}

} // end namespace transport
} // end namespace adios2
