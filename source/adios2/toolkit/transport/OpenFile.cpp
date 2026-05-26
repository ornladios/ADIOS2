/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "OpenFile.h"

#include <sstream>
#include <tuple>

#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosString.h"
#include "adios2/helper/adiosSystem.h" // CreateDirectory
#include "adios2/helper/adiosType.h"


/// transports
#include "adios2/toolkit/transport/file/FilePOSIX.h"
#ifdef ADIOS2_HAVE_DAOS
#include "adios2/toolkit/transport/file/FileDaos.h" // used by OpenFile and MkDirsBarrier
#endif
#ifdef ADIOS2_HAVE_SST
#include "adios2/toolkit/transport/file/FileRemote.h"
#endif
#ifdef ADIOS2_HAVE_IME
#include "adios2/toolkit/transport/file/FileIME.h"
#endif
#ifdef ADIOS2_HAVE_AWSSDK
#include "adios2/toolkit/transport/file/FileAWSSDK.h"
#endif

#ifdef _WIN32
#pragma warning(disable : 4503) // length of std::function inside std::async
#endif

#include "adios2/toolkit/transport/file/FileFStream.h"
#ifndef _WIN32
#include "adios2/toolkit/transport/file/FileHTTP.h"
#endif
#ifdef ADIOS2_HAVE_OPENSSL
#include "adios2/toolkit/transport/file/FileHTTPS.h"
#endif
#include "adios2/toolkit/transport/file/FileStdio.h"
#include "adios2/toolkit/transport/null/NullTransport.h"

namespace adios2
{
namespace transport
{

namespace
{

std::shared_ptr<Transport> OpenFileImpl(helper::Comm const &comm, const std::string &fileName,
                                        const Mode openMode, const Params &parameters_,
                                        const bool profile, const bool useComm,
                                        const helper::Comm &chainComm)
{
    // This function expects Params with lower case keys!!!
    const Params &parameters = helper::LowerCaseParams(parameters_);

    auto lf_GetBuffered = [&](const std::string bufferedDefault) -> bool {
        bool bufferedValue;
        std::string bufferedValueStr(bufferedDefault);
        helper::SetParameterValue("buffered", parameters, bufferedValueStr);
        {
            std::stringstream ss(bufferedValueStr);
            if (!(ss >> std::boolalpha >> bufferedValue))
            {
                helper::Throw<std::invalid_argument>("Toolkit", "transport", "OpenFile",
                                                     "invalid value for \"buffered\" transport "
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
            transport = std::make_shared<transport::FileStdio>(comm);
            if (!lf_GetBuffered("true"))
            {
                transport->SetBuffer(nullptr, 0);
            }
        }
        else if (library == "fstream")
        {
            transport = std::make_shared<transport::FileFStream>(comm);
            if (!lf_GetBuffered("true"))
            {
                transport->SetBuffer(nullptr, 0);
            }
        }
        else if (library == "posix")
        {
            transport = std::make_shared<transport::FilePOSIX>(comm);
            if (lf_GetBuffered("false"))
            {
                helper::Throw<std::invalid_argument>(
                    "Toolkit", "transport", "OpenFile",
                    library + " transport does not support buffered I/O.");
            }
        }
#ifdef ADIOS2_HAVE_DAOS
        else if (library == "daos")
        {
            transport = std::make_shared<transport::FileDaos>(comm);
            if (lf_GetBuffered("false"))
            {
                helper::Throw<std::invalid_argument>(
                    "Toolkit", "transport", "OpenFile",
                    library + " transport does not support buffered I/O.");
            }
        }
#endif
#ifdef ADIOS2_HAVE_SST
        else if (library == "remote")
        {
            transport = std::make_shared<transport::FileRemote>(comm);
            if (lf_GetBuffered("false"))
            {
                helper::Throw<std::invalid_argument>(
                    "Toolkit", "transport", "OpenFile",
                    library + " transport does not support buffered I/O.");
            }
        }
#endif
#ifdef ADIOS2_HAVE_IME
        else if (library == "ime")
        {
            transport = std::make_shared<transport::FileIME>(comm);
        }
#endif
#ifdef ADIOS2_HAVE_AWSSDK
        else if (library == "awssdk")
        {
            transport = std::make_shared<transport::FileAWSSDK>(comm);
        }
#endif
#ifndef _WIN32
        else if (library == "http")
        {
            transport = std::make_shared<transport::FileHTTP>(comm);
            if (lf_GetBuffered("false"))
            {
                helper::Throw<std::invalid_argument>(
                    "Toolkit", "transport", "OpenFile",
                    library + " transport does not support buffered I/O.");
            }
        }
#endif
#ifdef ADIOS2_HAVE_OPENSSL
        else if (library == "https")
        {
            transport = std::make_shared<transport::FileHTTPS>(comm);
            if (lf_GetBuffered("false"))
            {
                helper::Throw<std::invalid_argument>(
                    "Toolkit", "transport", "OpenFile",
                    library + " transport does not support buffered I/O.");
            }
        }
#endif
        else if (library == "null")
        {
            transport = std::make_shared<transport::NullTransport>(comm);
            if (lf_GetBuffered("false"))
            {
                helper::Throw<std::invalid_argument>(
                    "Toolkit", "transport", "OpenFile",
                    library + " transport does not support buffered I/O.");
            }
        }
        else
        {
            helper::Throw<std::invalid_argument>("Toolkit", "transport", "OpenFile",
                                                 "invalid IO AddTransport library " + library);
        }
    };

    auto lf_GetLibrary = [](const std::string defaultLibrary,
                            const Params &parameters) -> std::string {
        std::string library(defaultLibrary);
        helper::SetParameterValue("library", parameters, library);
        return library;
    };

    auto lf_GetTimeUnits = [&](const std::string defaultTimeUnit,
                               const Params &parameters) -> TimeUnit {
        std::string profileUnits(defaultTimeUnit);
        helper::SetParameterValue("profileunits", parameters, profileUnits);
        return helper::StringToTimeUnit(profileUnits);
    };

    auto lf_GetAsyncOpen = [&](const std::string defaultAsync, const Params &parameters) -> bool {
        std::string AsyncOpen = defaultAsync;
        helper::SetParameterValue("asyncopen", parameters, AsyncOpen);
        return helper::StringTo<bool>(AsyncOpen, "");
    };

    auto lf_GetDirectIO = [&](const std::string defaultValue, const Params &parameters) -> bool {
        std::string directio = defaultValue;
        helper::SetParameterValue("directio", parameters, directio);
        return helper::StringTo<bool>(directio, "");
    };

    auto lf_GetTarInfo = [&](const Params &parameters) -> std::tuple<size_t, size_t> {
        std::string baseOffset = "0";
        std::string baseSize = "0";
        helper::SetParameterValue("taroffset", parameters, baseOffset);
        helper::SetParameterValue("tarsize", parameters, baseSize);
        return std::make_tuple<size_t, size_t>(
            helper::StringToSizeT(baseOffset, "convert tar offset string to size_t"),
            helper::StringToSizeT(baseSize, "convert tar size string to size_t"));
    };

    // BODY OF FUNCTION starts here
    std::shared_ptr<Transport> transport;
    const std::string library = helper::LowerCase(lf_GetLibrary(DefaultFileLibrary, parameters));
    if (getenv("DoFileRemote") && (openMode == Mode::Read))
        lf_SetFileTransport("remote", transport);
    else
        lf_SetFileTransport(library, transport);

    // Default or user ProfileUnits in parameters
    if (profile)
    {
        transport->InitProfiler(openMode, lf_GetTimeUnits(DefaultTimeUnit, parameters));
    }

    // If "file" is actually in a TAR file, set BaseOffset and BaseSize
    auto pair = lf_GetTarInfo(parameters);
    transport->m_BaseOffset = std::get<0>(pair);
    transport->m_BaseSize = std::get<1>(pair);
    transport->SetParameters(parameters);

    // open
    if (useComm)
    {
        transport->OpenChain(fileName, openMode, chainComm, lf_GetAsyncOpen("false", parameters),
                             lf_GetDirectIO("false", parameters));
    }
    else
    {
        transport->Open(fileName, openMode, lf_GetAsyncOpen("false", parameters),
                        lf_GetDirectIO("false", parameters));
    }
    return transport;
}

} // end anonymous namespace

std::shared_ptr<Transport> OpenFile(helper::Comm const &comm, std::string const &name, Mode mode,
                                    Params const &parameters, bool profile)
{
    return OpenFileImpl(comm, name, mode, parameters, profile, /*useComm=*/false, comm);
}

std::shared_ptr<Transport> OpenFileChained(helper::Comm const &comm, std::string const &name,
                                           Mode mode, Params const &parameters, bool profile,
                                           helper::Comm const &chainComm)
{
    return OpenFileImpl(comm, name, mode, parameters, profile, /*useComm=*/true, chainComm);
}

void MkDirsBarrier(helper::Comm const &comm, const std::vector<std::string> &fileNames,
                   const std::vector<Params> &parametersVector, const bool nodeLocal)
{
    auto lf_CreateDirectories = [&](const std::vector<std::string> &fileNames) {
        for (size_t i = 0; i < fileNames.size(); ++i)
        {
            const auto lastPathSeparator(fileNames[i].find_last_of(PathSeparator));
            if (lastPathSeparator == std::string::npos)
            {
                continue;
            }
            const Params &parameters = parametersVector[i];
            const std::string type = parameters.at("transport");
            if (type == "File" || type == "file")
            {
                const std::string path(fileNames[i].substr(0, lastPathSeparator));

                std::string library;
                helper::SetParameterValue("Library", parameters, library);
                helper::SetParameterValue("library", parameters, library);
                if (library == "Daos" || library == "daos")
                {
#ifdef ADIOS2_HAVE_DAOS
                    auto transport = std::make_shared<transport::FileDaos>(comm);
                    transport->SetParameters({{"SingleProcess", "true"}});
                    transport->MkDir(path);
#endif
                }
                else
                {
#ifdef CreateDirectory
#undef CreateDirectory
#endif
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
        int rank = comm.Rank();
        if (rank == 0)
        {
            lf_CreateDirectories(fileNames);
        }

        comm.Barrier("Barrier in transport::MkDirsBarrier");
    }
}

} // end namespace transport
} // end namespace adios2
