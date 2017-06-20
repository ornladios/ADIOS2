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
#include "adios2/toolkit/transport/file/FileDescriptor.h"
#include "adios2/toolkit/transport/file/FilePointer.h"
#include "adios2/toolkit/transport/file/FileStream.h"

namespace adios2
{
namespace transportman
{

TransportMan::TransportMan(MPI_Comm mpiComm, const bool debugMode)
: m_MPIComm(mpiComm), m_DebugMode(debugMode)
{
}

void TransportMan::OpenFiles(const std::vector<std::string> &baseNames,
                             const std::vector<std::string> &names,
                             const OpenMode openMode,
                             const std::vector<Params> &parametersVector,
                             const bool profile)
{
    const unsigned int size = baseNames.size();

    for (unsigned int i = 0; i < size; ++i)
    {
        const Params &parameters = parametersVector[i];
        const std::string type(parameters.at("transport"));

        if (type == "File" || type == "file") // need to create directory
        {
            CreateDirectory(baseNames[i]);
            OpenFileTransport(names[i], openMode, parameters, profile);
        }
    }
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

        const std::string type(parameters.at("Transport"));
        auto itType = typeTransportNames.find(type);

        if (m_DebugMode)
        {
            // check if name exists for this transport type
            if (itType->second.count(name) == 1)
            {
                throw std::invalid_argument(
                    "ERROR: two IO AddTransport of the same type can't "
                    "have the same name : " +
                    name + ", use Name=value parameter, in "
                           "call to Open");
            }
        }
        itType->second.insert(name);
        baseNames.push_back(name);
    }
    return baseNames;
}

bool TransportMan::CheckTransportIndex(const int index) const noexcept
{
    const int upperLimit = static_cast<int>(m_Transports.size());
    const int lowerLimit = -1;
    return CheckIndexRange(index, upperLimit, lowerLimit);
}

std::vector<std::string> TransportMan::GetTransportsTypes() noexcept
{
    std::vector<std::string> types;
    types.reserve(m_Transports.size());

    for (const auto &transport : m_Transports)
    {
        types.push_back(transport->m_Type + "_" + transport->m_Library);
    }
    return types;
}

std::vector<profiling::IOChrono *>
TransportMan::GetTransportsProfilers() noexcept
{
    std::vector<profiling::IOChrono *> profilers;
    profilers.reserve(m_Transports.size());

    for (const auto &transport : m_Transports)
    {
        profilers.push_back(&transport->m_Profiler);
    }
    return profilers;
}

void TransportMan::WriteFiles(const char *buffer, const size_t size,
                              const int transportIndex)
{
    if (transportIndex == -1)
    {
        for (auto &transport : m_Transports)
        {
            if (transport->m_Type == "File")
            {
                // make this truly asynch?
                transport->Write(buffer, size);
            }
        }
    }
    else
    {
        if (m_DebugMode)
        {
            if (m_Transports[transportIndex]->m_Type != "File")
            {
                throw std::invalid_argument(
                    "ERROR: index " + std::to_string(transportIndex) +
                    " doesn't come from a file transport in IO AddTransport, "
                    "in call to Write (flush) or Close\n");
            }
        }

        m_Transports[transportIndex]->Write(buffer, size);
    }
}

void TransportMan::CloseFiles(const int transportIndex)
{
    if (transportIndex == -1)
    {
        for (auto &transport : m_Transports)
        {
            if (transport->m_Type == "File")
            {
                transport->Close();
            }
        }
    }
    else
    {
        if (m_DebugMode)
        {
            if (m_Transports[transportIndex]->m_Type != "File")
            {
                throw std::invalid_argument(
                    "ERROR: index " + std::to_string(transportIndex) +
                    " doesn't come from a file transport in IO AddTransport, "
                    "in call to Close\n");
            }
        }

        m_Transports[transportIndex]->Close();
    }
}

bool TransportMan::AllTransportsClosed() const noexcept
{
    bool allClose = true;
    for (const auto &transport : m_Transports)
    {
        if (transport->m_IsOpen)
        {
            allClose = false;
            break;
        }
    }
    return allClose;
}

// PRIVATE
void TransportMan::OpenFileTransport(const std::string &fileName,
                                     const OpenMode openMode,
                                     const Params &parameters,
                                     const bool profile)
{
    auto lf_SetFileTransport = [&](const std::string library,
                                   std::shared_ptr<Transport> &transport) {
        if (library == "POSIX")
        {
            transport = std::make_shared<transport::FileDescriptor>(
                m_MPIComm, m_DebugMode);
        }
        else if (library == "stdio")
        {
            transport = std::make_shared<transport::FilePointer>(m_MPIComm,
                                                                 m_DebugMode);
        }
        else if (library == "fstream")
        {
            transport =
                std::make_shared<transport::FileStream>(m_MPIComm, m_DebugMode);
        }
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

    // open and move transport to container
    transport->Open(fileName, openMode);
    m_Transports.push_back(std::move(transport)); // is move needed?
}

} // end namespace transport
} // end namespace adios
