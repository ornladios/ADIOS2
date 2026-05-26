/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TransportMan.h"

#include <ios>
#include <iostream>
#include <set>

#include "adios2/helper/adiosFunctions.h" //CreateDirectory
#include "adios2/toolkit/transport/OpenFile.h"
#include <adios2sys/SystemTools.hxx>

#ifdef _WIN32
#pragma warning(disable : 4503) // length of std::function inside std::async
#endif

namespace adios2
{
namespace transportman
{

TransportMan::TransportMan(core::IO &io, helper::Comm &comm) : m_IO(io), m_Comm(comm) {}

void TransportMan::MkDirsBarrier(const std::vector<std::string> &fileNames,
                                 const std::vector<Params> &parametersVector, const bool nodeLocal)
{
    transport::MkDirsBarrier(m_Comm, fileNames, parametersVector, nodeLocal);
}

void TransportMan::OpenFiles(const std::vector<std::string> &fileNames, const Mode openMode,
                             const std::vector<Params> &parametersVector, const bool profile)
{
    for (size_t i = 0; i < fileNames.size(); ++i)
    {
        const Params &parameters = helper::LowerCaseParams(parametersVector[i]);
        const std::string type = helper::LowerCase(parameters.at("transport"));

        if (type == "file")
        {
            std::shared_ptr<Transport> file =
                OpenFileTransport(fileNames[i], openMode, parameters, profile, false, m_Comm);
            m_Transports.insert({i, file});
        }
    }
}

void TransportMan::OpenFiles(const std::vector<std::string> &fileNames, const Mode openMode,
                             const std::vector<Params> &parametersVector, const bool profile,
                             const helper::Comm &chainComm)
{
    for (size_t i = 0; i < fileNames.size(); ++i)
    {
        const Params &parameters = helper::LowerCaseParams(parametersVector[i]);
        const std::string type = helper::LowerCase(parameters.at("transport"));

        if (type == "file")
        {
            std::shared_ptr<Transport> file =
                OpenFileTransport(fileNames[i], openMode, parameters, profile, true, chainComm);
            m_Transports.insert({i, file});
        }
    }
}

void TransportMan::OpenFileID(const std::string &name, const size_t id, const Mode mode,
                              const Params &parameters, const bool profile)
{
    std::shared_ptr<Transport> file =
        OpenFileTransport(name, mode, helper::LowerCaseParams(parameters), profile, false, m_Comm);
    m_Transports.insert({id, file});
}

std::vector<std::string>
TransportMan::GetFilesBaseNames(const std::string &baseName,
                                const std::vector<Params> &parametersVector)
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
                helper::Throw<std::invalid_argument>("Toolkit", "TransportMan", "OpenFileID",
                                                     "two IO AddTransport of the same type can't "
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

std::vector<profiling::IOChrono *> TransportMan::GetTransportsProfilers() noexcept
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

std::vector<std::string> TransportMan::GetTransportsNames() noexcept
{
    std::vector<std::string> names;
    names.reserve(m_Transports.size());

    for (const auto &transportPair : m_Transports)
    {
        const std::shared_ptr<Transport> &transport = transportPair.second;
        names.push_back(adios2sys::SystemTools::GetFilenameName(transport->m_Name));
    }
    return names;
}

void TransportMan::WriteFiles(const char *buffer, const size_t size, const int transportIndex)
{
    if (transportIndex == -1)
    {
        for (auto &transportPair : m_Transports)
        {
            auto &transport = transportPair.second;
            if (transport->m_Type == "File")
            {
                transport->Write(buffer, size);
            }
        }
    }
    else
    {
        auto itTransport = m_Transports.find(transportIndex);
        CheckFile(itTransport,
                  ", in call to WriteFiles with index " + std::to_string(transportIndex));
        itTransport->second->Write(buffer, size);
    }
}

void TransportMan::WriteFileAt(const char *buffer, const size_t size, const size_t start,
                               const int transportIndex)
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
        CheckFile(itTransport,
                  ", in call to WriteFileAt with index " + std::to_string(transportIndex));
        itTransport->second->Write(buffer, size, start);
    }
}

void TransportMan::WriteFiles(const core::iovec *iov, const size_t iovcnt, const int transportIndex)
{
    if (transportIndex == -1)
    {
        for (auto &transportPair : m_Transports)
        {
            auto &transport = transportPair.second;
            if (transport->m_Type == "File")
            {
                transport->WriteV(iov, static_cast<int>(iovcnt));
            }
        }
    }
    else
    {
        auto itTransport = m_Transports.find(transportIndex);
        CheckFile(itTransport,
                  ", in call to WriteFiles with index " + std::to_string(transportIndex));
        itTransport->second->WriteV(iov, static_cast<int>(iovcnt));
    }
}

void TransportMan::WriteFileAt(const core::iovec *iov, const size_t iovcnt, const size_t start,
                               const int transportIndex)
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
        CheckFile(itTransport,
                  ", in call to WriteFileAt with index " + std::to_string(transportIndex));
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
        CheckFile(itTransport,
                  ", in call to SeekToFileEnd with index " + std::to_string(transportIndex));
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
        CheckFile(itTransport,
                  ", in call to SeekToFileBegin with index " + std::to_string(transportIndex));
        itTransport->second->SeekToBegin();
    }
}

void TransportMan::SeekTo(const size_t start, const int transportIndex)
{
    if (transportIndex == -1)
    {
        for (auto &transportPair : m_Transports)
        {
            auto &transport = transportPair.second;
            if (transport->m_Type == "File")
            {
                transport->Seek(start);
            }
        }
    }
    else
    {
        auto itTransport = m_Transports.find(transportIndex);
        CheckFile(itTransport, ", in call to SeekTo with index " + std::to_string(transportIndex));
        itTransport->second->Seek(start);
    }
}

size_t TransportMan::CurrentPos(const int transportIndex)
{
    auto itTransport = m_Transports.find(transportIndex);
    CheckFile(itTransport, ", in call to CurrentPos with index " + std::to_string(transportIndex));
    return itTransport->second->CurrentPos();
}

void TransportMan::Truncate(const size_t length, const int transportIndex)
{
    if (transportIndex == -1)
    {
        for (auto &transportPair : m_Transports)
        {
            auto &transport = transportPair.second;
            if (transport->m_Type == "File")
            {
                transport->Truncate(length);
            }
        }
    }
    else
    {
        auto itTransport = m_Transports.find(transportIndex);
        CheckFile(itTransport,
                  ", in call to Truncate with index " + std::to_string(transportIndex));
        itTransport->second->Truncate(length);
    }
}

size_t TransportMan::GetFileSize(const size_t transportIndex) const
{
    auto itTransport = m_Transports.find(transportIndex);
    CheckFile(itTransport, ", in call to GetFileSize with index " + std::to_string(transportIndex));
    return itTransport->second->GetSize();
}

void TransportMan::ReadFile(char *buffer, const size_t size, const size_t start,
                            const size_t transportIndex)
{
    auto itTransport = m_Transports.find(transportIndex);
    CheckFile(itTransport, ", in call to ReadFile with index " + std::to_string(transportIndex));
    itTransport->second->Read(buffer, size, start);
}

void TransportMan::SetParameters(const Params &params, const int transportIndex)
{
    if (transportIndex == -1)
    {
        for (auto &transportPair : m_Transports)
        {
            auto &transport = transportPair.second;

            transport->SetParameters(params);
        }
    }
    else
    {
        auto itTransport = m_Transports.find(transportIndex);
        CheckFile(itTransport,
                  ", in call to SetParameters with index " + std::to_string(transportIndex));
        itTransport->second->SetParameters(params);
    }
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
        CheckFile(itTransport,
                  ", in call to FlushFiles with index " + std::to_string(transportIndex));
        itTransport->second->Flush();
    }
}

void TransportMan::FinalizeSegment()
{
    for (auto &transportPair : m_Transports)
    {
        auto &transport = transportPair.second;
        if (transport->m_Type == "File")
        {
            transport->FinalizeSegment();
        }
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
        CheckFile(itTransport,
                  ", in call to CloseFiles with index " + std::to_string(transportIndex));
        itTransport->second->Close();
        m_Transports.erase(itTransport);
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
        CheckFile(itTransport,
                  ", in call to CloseFiles with index " + std::to_string(transportIndex));
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

bool TransportMan::FileExists(const std::string &name, const Params &parameters, const bool profile)
{
    bool exists = false;
    try
    {
        std::shared_ptr<Transport> file = OpenFileTransport(
            name, Mode::Read, helper::LowerCaseParams(parameters), profile, false, m_Comm);
        exists = true;
        file->Close();
    }
    catch (std::ios_base::failure &)
    {
    }
    return exists;
}

// PRIVATE
std::shared_ptr<Transport> TransportMan::OpenFileTransport(const std::string &fileName,
                                                           const Mode openMode,
                                                           const Params &parameters_,
                                                           const bool profile, const bool useComm,
                                                           const helper::Comm &chainComm)
{
    if (useComm)
    {
        return transport::OpenFileChained(m_Comm, fileName, openMode, parameters_, profile,
                                          chainComm);
    }
    return transport::OpenFile(m_Comm, fileName, openMode, parameters_, profile);
}

void TransportMan::CheckFile(
    std::unordered_map<size_t, std::shared_ptr<Transport>>::const_iterator itTransport,
    const std::string hint) const
{
    if (itTransport == m_Transports.end())
    {
        helper::Throw<std::invalid_argument>("Toolkit", "TransportMan", "CheckFile",
                                             "invalid transport " + hint);
    }

    if (itTransport->second->m_Type != "File")
    {
        helper::Throw<std::invalid_argument>("Toolkit", "TransportMan", "CheckFile",
                                             "invalid type " + itTransport->second->m_Library +
                                                 ", must be file " + hint);
    }
}

} // end namespace transport
} // end namespace adios2
