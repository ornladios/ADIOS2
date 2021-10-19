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

#include <ios>
#include <iostream>
#include <set>

#include "adios2/helper/adiosFunctions.h" //CreateDirectory

/// transports
#ifndef _WIN32
#include "adios2/toolkit/transport/file/FilePOSIX.h"
#endif
#ifdef ADIOS2_HAVE_DAOS
#include "adios2/toolkit/transport/file/FileDaos.h"
#endif
#ifdef ADIOS2_HAVE_IME
#include "adios2/toolkit/transport/file/FileIME.h"
#endif

#ifdef _WIN32
#pragma warning(disable : 4503) // length of std::function inside std::async
#endif

#include "adios2/toolkit/transport/file/FileFStream.h"
#include "adios2/toolkit/transport/file/FileStdio.h"
#include "adios2/toolkit/transport/null/NullTransport.h"

namespace adios2
{
namespace transportman
{

TransportMan::TransportMan(helper::Comm &comm) : m_Comm(comm) {}

void TransportMan::MkDirsBarrier(const std::vector<std::string> &fileNames,
                                 const std::vector<Params> &parametersVector,
                                 const bool nodeLocal)
{
    auto lf_CreateDirectories = [&](const std::vector<std::string> &fileNames) {
        for (size_t i = 0; i < fileNames.size(); ++i)
        {
            const auto lastPathSeparator(
                fileNames[i].find_last_of(PathSeparator));
            if (lastPathSeparator == std::string::npos)
            {
                continue;
            }
            const Params &parameters = parametersVector[i];
            const std::string type = parameters.at("transport");
            if (type == "File" || type == "file")
            {
                const std::string path(
                    fileNames[i].substr(0, lastPathSeparator));

                std::string library;
                helper::SetParameterValue("Library", parameters, library);
                helper::SetParameterValue("library", parameters, library);
                if (library == "Daos" || library == "daos")
                {
#ifdef ADIOS2_HAVE_DAOS
                    auto transport =
                        std::make_shared<transport::FileDaos>(m_Comm);
                    transport->SetParameters({{"SingleProcess", "true"}});
                    // int rank = m_Comm.Rank();
                    // std::cout << "rank " << rank << ": start
                    // transport->MkDir(" << path << ")..." << std::endl;
                    transport->MkDir(path);
                    // std::cout << "rank " << rank << ": transport->MkDir(" <<
                    // path << ") succeeded!" << std::endl;
#endif
                }
                else
                {
                    helper::CreateDirectory(path);
                }
            }
        }
    };

    if (nodeLocal)
    {
        lf_CreateDirectories(fileNames);
    }
    else
    {
        int rank = m_Comm.Rank();
        if (rank == 0)
        {
            lf_CreateDirectories(fileNames);
        }

        m_Comm.Barrier("Barrier in TransportMan.MkDirsBarrier");
    }
}

void TransportMan::OpenFiles(const std::vector<std::string> &fileNames,
                             const Mode openMode,
                             const std::vector<Params> &parametersVector,
                             const bool profile)
{
    for (size_t i = 0; i < fileNames.size(); ++i)
    {
        const Params &parameters = parametersVector[i];
        const std::string type = parameters.at("transport");

        if (type == "File" || type == "file")
        {
            std::shared_ptr<Transport> file = OpenFileTransport(
                fileNames[i], openMode, parameters, profile, false, m_Comm);
            m_Transports.insert({i, file});
        }
    }
}

void TransportMan::OpenFiles(const std::vector<std::string> &fileNames,
                             const Mode openMode,
                             const std::vector<Params> &parametersVector,
                             const bool profile, const helper::Comm &chainComm)
{
    for (size_t i = 0; i < fileNames.size(); ++i)
    {
        const Params &parameters = parametersVector[i];
        const std::string type = parameters.at("transport");

        if (type == "File" || type == "file")
        {
            std::shared_ptr<Transport> file = OpenFileTransport(
                fileNames[i], openMode, parameters, profile, true, chainComm);
            m_Transports.insert({i, file});
        }
    }
}

void TransportMan::OpenFileID(const std::string &name, const size_t id,
                              const Mode mode, const Params &parameters,
                              const bool profile)
{
    std::shared_ptr<Transport> file =
        OpenFileTransport(name, mode, parameters, profile, false, m_Comm);
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
        helper::SetParameterValue("Name", parameters, name); // if found in map

        const std::string type(parameters.at("transport"));

        auto itType = typeTransportNames.find(type);
        // check if name exists for this transport type
        if (itType != typeTransportNames.end())
        {
            if (itType->second.count(name) == 1)
            {
                throw std::invalid_argument(
                    "ERROR: two IO AddTransport of the same type can't "
                    "have the same name : " +
                    name +
                    ", use Name=value parameter, in "
                    "call to Open");
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

void TransportMan::WriteFileAt(const char *buffer, const size_t size,
                               const size_t start, const int transportIndex)
{
    if (transportIndex == -1)
    {
        for (auto &transportPair : m_Transports)
        {
            auto &transport = transportPair.second;
            if (transport->m_Type == "File")
            {
                transport->Write(buffer, size, start);
            }
        }
    }
    else
    {
        auto itTransport = m_Transports.find(transportIndex);
        CheckFile(itTransport, ", in call to WriteFileAt with index " +
                                   std::to_string(transportIndex));
        itTransport->second->Write(buffer, size, start);
    }
}

void TransportMan::WriteFiles(const core::iovec *iov, const size_t iovcnt,
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
                transport->WriteV(iov, static_cast<int>(iovcnt));
            }
        }
    }
    else
    {
        auto itTransport = m_Transports.find(transportIndex);
        CheckFile(itTransport, ", in call to WriteFiles with index " +
                                   std::to_string(transportIndex));
        itTransport->second->WriteV(iov, static_cast<int>(iovcnt));
    }
}

void TransportMan::WriteFileAt(const core::iovec *iov, const size_t iovcnt,
                               const size_t start, const int transportIndex)
{
    if (transportIndex == -1)
    {
        for (auto &transportPair : m_Transports)
        {
            auto &transport = transportPair.second;
            if (transport->m_Type == "File")
            {
                transport->WriteV(iov, static_cast<int>(iovcnt), start);
            }
        }
    }
    else
    {
        auto itTransport = m_Transports.find(transportIndex);
        CheckFile(itTransport, ", in call to WriteFileAt with index " +
                                   std::to_string(transportIndex));
        itTransport->second->WriteV(iov, static_cast<int>(iovcnt), start);
    }
}

void TransportMan::SeekToFileEnd(const int transportIndex)
{
    if (transportIndex == -1)
    {
        for (auto &transportPair : m_Transports)
        {
            auto &transport = transportPair.second;
            if (transport->m_Type == "File")
            {
                transport->SeekToEnd();
            }
        }
    }
    else
    {
        auto itTransport = m_Transports.find(transportIndex);
        CheckFile(itTransport, ", in call to SeekToFileEnd with index " +
                                   std::to_string(transportIndex));
        itTransport->second->SeekToEnd();
    }
}

void TransportMan::SeekToFileBegin(const int transportIndex)
{
    if (transportIndex == -1)
    {
        for (auto &transportPair : m_Transports)
        {
            auto &transport = transportPair.second;
            if (transport->m_Type == "File")
            {
                transport->SeekToBegin();
            }
        }
    }
    else
    {
        auto itTransport = m_Transports.find(transportIndex);
        CheckFile(itTransport, ", in call to SeekToFileBegin with index " +
                                   std::to_string(transportIndex));
        itTransport->second->SeekToBegin();
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

void TransportMan::FlushFiles(const int transportIndex)
{
    if (transportIndex == -1)
    {
        for (auto &transportPair : m_Transports)
        {
            auto &transport = transportPair.second;

            if (transport->m_Type == "File")
            {
                transport->Flush();
            }
        }
    }
    else
    {
        auto itTransport = m_Transports.find(transportIndex);
        CheckFile(itTransport, ", in call to FlushFiles with index " +
                                   std::to_string(transportIndex));
        itTransport->second->Flush();
    }
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

void TransportMan::DeleteFiles(const int transportIndex)
{
    if (transportIndex == -1)
    {
        for (auto &transportPair : m_Transports)
        {
            auto &transport = transportPair.second;

            if (transport->m_Type == "File")
            {
                transport->Delete();
            }
        }
    }
    else
    {
        auto itTransport = m_Transports.find(transportIndex);
        CheckFile(itTransport, ", in call to CloseFiles with index " +
                                   std::to_string(transportIndex));
        itTransport->second->Delete();
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

bool TransportMan::FileExists(const std::string &name, const Params &parameters,
                              const bool profile)
{
    bool exists = false;
    try
    {
        std::shared_ptr<Transport> file = OpenFileTransport(
            name, Mode::Read, parameters, profile, false, m_Comm);
        exists = true;
        file->Close();
    }
    catch (std::ios_base::failure &)
    {
    }
    return exists;
}

// PRIVATE
std::shared_ptr<Transport> TransportMan::OpenFileTransport(
    const std::string &fileName, const Mode openMode, const Params &parameters,
    const bool profile, const bool useComm, const helper::Comm &chainComm)
{
    auto lf_GetBuffered = [&](const std::string bufferedDefault) -> bool {
        bool bufferedValue;
        std::string bufferedValueStr(bufferedDefault);
        helper::SetParameterValue("Buffered", parameters, bufferedValueStr);
        helper::SetParameterValue("buffered", parameters, bufferedValueStr);
        {
            std::stringstream ss(bufferedValueStr);
            if (!(ss >> std::boolalpha >> bufferedValue))
            {
                throw std::invalid_argument(
                    "ERROR: invalid value for \"buffered\" transport "
                    "parameter: " +
                    bufferedValueStr);
            }
        }
        return bufferedValue;
    };

    auto lf_SetFileTransport = [&](const std::string library,
                                   std::shared_ptr<Transport> &transport) {
        if (library == "stdio")
        {
            transport = std::make_shared<transport::FileStdio>(m_Comm);
            if (!lf_GetBuffered("true"))
            {
                transport->SetBuffer(nullptr, 0);
            }
        }
        else if (library == "fstream")
        {
            transport = std::make_shared<transport::FileFStream>(m_Comm);
            if (!lf_GetBuffered("true"))
            {
                transport->SetBuffer(nullptr, 0);
            }
        }
#ifndef _WIN32
        else if (library == "POSIX" || library == "posix")
        {
            transport = std::make_shared<transport::FilePOSIX>(m_Comm);
            if (lf_GetBuffered("false"))
            {
                throw std::invalid_argument(
                    "ERROR: " + library +
                    " transport does not support buffered I/O.");
            }
        }
#endif
#ifdef ADIOS2_HAVE_DAOS
        else if (library == "Daos" || library == "daos")
        {
            transport = std::make_shared<transport::FileDaos>(m_Comm);
            if (lf_GetBuffered("false"))
            {
                throw std::invalid_argument(
                    "ERROR: " + library +
                    " transport does not support buffered I/O.");
            }
        }
#endif
#ifdef ADIOS2_HAVE_IME
        else if (library == "IME" || library == "ime")
        {
            transport = std::make_shared<transport::FileIME>(m_Comm);
        }
#endif
        else if (library == "NULL" || library == "null")
        {
            transport = std::make_shared<transport::NullTransport>(m_Comm);
            if (lf_GetBuffered("false"))
            {
                throw std::invalid_argument(
                    "ERROR: " + library +
                    " transport does not support buffered I/O.");
            }
        }
        else
        {
            throw std::invalid_argument(
                "ERROR: invalid IO AddTransport library " + library);
        }
    };

    auto lf_GetLibrary = [](const std::string defaultLibrary,
                            const Params &parameters) -> std::string {
        std::string library(defaultLibrary);
        helper::SetParameterValue("Library", parameters, library);
        helper::SetParameterValue("library", parameters, library);
        return library;
    };

    auto lf_GetTimeUnits = [&](const std::string defaultTimeUnit,
                               const Params &parameters) -> TimeUnit {
        std::string profileUnits(defaultTimeUnit);
        helper::SetParameterValue("ProfileUnits", parameters, profileUnits);
        helper::SetParameterValue("profileunits", parameters, profileUnits);
        return helper::StringToTimeUnit(profileUnits);
    };

    auto lf_GetAsync = [&](const std::string defaultAsync,
                           const Params &parameters) -> bool {
        std::string Async = defaultAsync;
        helper::SetParameterValue("AsyncTasks", parameters, Async);
        helper::SetParameterValue("asynctasks", parameters, Async);
        return helper::StringTo<bool>(Async, "");
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

    transport->SetParameters(parameters);

    // open
    if (useComm)
    {
        transport->OpenChain(fileName, openMode, chainComm,
                             lf_GetAsync("true", parameters));
    }
    else
    {
        transport->Open(fileName, openMode, lf_GetAsync("false", parameters));
    }
    return transport;
}

void TransportMan::CheckFile(
    std::unordered_map<size_t, std::shared_ptr<Transport>>::const_iterator
        itTransport,
    const std::string hint) const
{
    if (itTransport == m_Transports.end())
    {
        throw std::invalid_argument("ERROR: invalid transport " + hint + "\n");
    }

    if (itTransport->second->m_Type != "File")
    {
        throw std::invalid_argument("ERROR: invalid type " +
                                    itTransport->second->m_Library +
                                    ", must be file " + hint + "\n");
    }
}

} // end namespace transport
} // end namespace adios2
