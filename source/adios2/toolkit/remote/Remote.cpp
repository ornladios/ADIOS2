/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Remote.h"
#include "EVPathRemote.h"
#include "adios2/core/ADIOS.h"
#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosNetwork.h"
#include "adios2/helper/adiosString.h"
#include "adios2/helper/adiosSystem.h"

#include "adios2/toolkit/remote/EVPathRemote.h"
#include "adios2/toolkit/remote/XrootdHttpRemote.h"
#include "adios2/toolkit/remote/XrootdRemote.h"

#ifdef _MSC_VER
#define strdup(x) _strdup(x)
#define strtok_r(str, delim, saveptr) strtok_s(str, delim, saveptr)
#endif

#define ThrowUp(x)                                                                                 \
    helper::Throw<std::invalid_argument>("Core", "Engine", "ThrowUp",                              \
                                         "Non-overridden function " + std::string(x) +             \
                                             " called in Remote")

namespace adios2
{

void Remote::Open(const std::string hostname, const int32_t port, const std::string filename,
                  const Mode mode, bool RowMajorOrdering, const adios2::Params &params)
{
    ThrowUp(("RemoteOpen"));
};

void Remote::OpenSimpleFile(const std::string hostname, const int32_t port,
                            const std::string filename)
{
    ThrowUp("RemoteSimpleOpen");
};

void Remote::OpenReadSimpleFile(const std::string hostname, const int32_t port,
                                const std::string filename, std::vector<char> &contents)
{
    ThrowUp("RemoteSimpleOpenRead");
};

Remote::GetHandle Remote::Get(const char *VarName, size_t Step, size_t StepCount, size_t BlockID,
                              Dims &Count, Dims &Start, Accuracy &accuracy, void *dest,
                              size_t /*destSize*/)
{
    ThrowUp("RemoteGet");
    return (Remote::GetHandle)(intptr_t)0;
};

bool Remote::WaitForGet(GetHandle handle)
{
    ThrowUp("RemoteWaitForGet");
    return false;
}

Remote::GetHandle Remote::Read(size_t Start, size_t Size, void *Dest)
{
    ThrowUp("RemoteRead");
    return (Remote::GetHandle)0;
};

void Remote::Close() { ThrowUp("RemoteClose"); };

Remote::~Remote() {}
Remote::Remote() {}
Remote::Remote(const RemoteSetup &remoteSetup) : m_RemoteSetup(remoteSetup) {}

int Remote::LaunchRemoteServerViaConnectionManager(const std::string remoteHost)
{
    if (remoteHost.empty() || remoteHost == "localhost")
    {
        // std::cout << "Remote::LaunchRemoteServerViaConnectionManager: Assume server is already "
        //              "running at on localhost at port = "
        //           << 26200 << std::endl;
        return 26200;
    }

    const adios2::HostOptions &hostOptions = core::ADIOS::GetHostOptions();
    helper::NetworkSocket socket;
    socket.Connect("localhost", 30000);

    struct adios2::HostConfig *hostconf = nullptr;

    auto it = hostOptions.find(remoteHost);
    if (it != hostOptions.end())
    {
        for (auto &ho : it->second)
        {
            if (ho.protocol == HostAccessProtocol::SSH)
            {
                hostconf = const_cast<HostConfig *>(&ho);
            }
        }
    }
    if (!hostconf)
    {
        helper::Throw<std::invalid_argument>(
            "Toolkit", "Remote", "EstablishConnection",
            "No ssh configuration found for host " + remoteHost +
                ". Add config in ~/.config/hpc-campaign/hosts.yaml");
    }

    std::string request = "/run_service?group=" + remoteHost + "&service=" + hostconf->name;

    char response[2048];
    socket.RequestResponse(request, response, 2048);

    // responses:
    //   port:-1,msg:incomplete_service_definition
    //   port:-1,msg:missing_service_in_request
    //   port:26200,cookie:0xd93d91e3643c9869,msg:no_error

    char *token;
    char *rest = response;

    int serverPort = -1;
    std::string cookie;

    // std::cout << "Response = \"" << response << "\"" << std::endl;
    while ((token = strtok_r(rest, ",", &rest)))
    {
        char *key;
        char *value = token;
        key = strtok_r(value, ":", &value);
        if (!strncmp(key, "port", 4))
        {
            serverPort = atoi(value);
        }
        else if (!strncmp(key, "cookie", 6))
        {
            cookie = std::string(value);
        }
        else if (!strncmp(key, "msg", 3))
        {
            if (strcmp(value, "no_error"))
            {
                helper::Throw<std::invalid_argument>("Toolkit", "Remote", "EstablishConnection",
                                                     "Error response from connection manager: " +
                                                         std::string(value));
            }
        }
        else
        {
            helper::Throw<std::invalid_argument>(
                "Toolkit", "Remote", "EstablishConnection",
                "Invalid response from connection manager. Do not understand key " +
                    std::string(key));
        }
    }

    socket.Close();
    return serverPort;
}

std::string Remote::GetKeyFromConnectionManager(const std::string keyID)
{
    helper::NetworkSocket socket;
    socket.Connect("localhost", 30000);
    std::string request = "/get_key?id=" + keyID;

    char response[2048];
    socket.RequestResponse(request, response, 2048);

    // responses:
    //   "port:-1,msg:incomplete_service_definition
    //   "key:0,msg:missing_key_id_in_request"
    //   "key:0,msg:cannot_find_key_id"
    //   "key:5980a49cb5e0c4a6042f73f7ce0277d3a577e1af9cfc530e4f5683a615815d6a,msg:no_error"

    char *token;
    char *rest = response;

    std::string keyhex;

    // std::cout << "Response from Connection manager = \"" << response << "\"" << std::endl;
    while ((token = strtok_r(rest, ",", &rest)))
    {
        char *key;
        char *value = token;
        key = strtok_r(value, ":", &value);
        if (!strncmp(key, "port", 4))
        {
            ; // we don't care about port, msg will throw the error message
        }
        else if (!strncmp(key, "key", 3))
        {
            keyhex = std::string(value);
        }
        else if (!strncmp(key, "msg", 3))
        {
            if (strcmp(value, "no_error") && strcmp(value, "cannot_find_key_id"))
            {
                helper::Throw<std::invalid_argument>("Toolkit", "Remote", "GetKey",
                                                     "Error response from connection manager: " +
                                                         std::string(value));
            }
        }
        else
        {
            helper::Throw<std::invalid_argument>(
                "Toolkit", "Remote", "EstablishConnection",
                "Invalid response from connection manager. Do not understand key " +
                    std::string(key));
        }
    }

    socket.Close();
    return keyhex;
}

std::string ParamsToEncodedString(const adios2::Params &params)
{
    std::string pstr;
    if (params.size() > 0)
    {
        for (const auto &p : params)
        {
            if (!pstr.empty())
                pstr += EVPathRemoteCommon::EngineParametersSeparator;
            pstr += p.first + "=" + p.second;
        }
    }
    return pstr;
}

adios2::Params EncodedStringToParams(const std::string &pstr)
{
    adios2::Params p;
    if (pstr.empty())
        return p;
    auto entries = helper::StringToVector(pstr, EVPathRemoteCommon::EngineParametersSeparator);
    for (auto const &e : entries)
    {
        auto pair = helper::StringToVector(e, '=');
        if (pair.size() == 2 && !pair[0].empty())
            p.emplace(pair[0], pair[1]);
    }
    return p;
}

RemoteSetup GetRemoteSetup(const std::string &remoteHost)
{
    const adios2::HostOptions &hostOptions = core::ADIOS::GetHostOptions();
    RemoteSetup rs;
    if (!remoteHost.empty())
    {
        rs.hostName = remoteHost;
        auto it = hostOptions.find(rs.hostName);
        if (it != hostOptions.end())
        {
            for (auto &hc : it->second)
            {
                if (hc.protocol == HostAccessProtocol::SSH)
                {
                    rs.protocol = hc.protocol;
                    rs.hostConfig = const_cast<HostConfig *>(&hc);
                    break;
                }
                if (hc.protocol == HostAccessProtocol::XRootD)
                {
                    rs.protocol = hc.protocol;
                    rs.hostConfig = const_cast<HostConfig *>(&hc);
                    rs.xrootdTransferProtocol = hc.transfer_protocol;
                    break;
                }
            }
        }
    }
    else
    {
        if (getenv("DoXRootDXrdCl"))
        {
            // XrdCl client against the same HTTPS server (reuses XRootDHttpsHost).
            char *env = getenv("XRootDHttpsHost");
            if (env)
                rs.hostName = std::string(env);
            rs.protocol = HostAccessProtocol::XRootD;
            rs.xrootdTransferProtocol = XRootDTransferProtocol::XrdCl;
        }
        else if (getenv("DoXRootDHttps"))
        {
            char *env = getenv("XRootDHttpsHost");
            if (env)
                rs.hostName = std::string(env);
            rs.protocol = HostAccessProtocol::XRootD;
            rs.xrootdTransferProtocol = XRootDTransferProtocol::HTTPS;
        }
        else if (getenv("DoXRootDHttp"))
        {
            char *env = getenv("XRootDHttpHost");
            if (env)
                rs.hostName = getenv("XRootDHttpHost");
            rs.protocol = HostAccessProtocol::XRootD;
            rs.xrootdTransferProtocol = XRootDTransferProtocol::HTTP;
        }
        else if (getenv("DoXRootD"))
        {
            char *env = getenv("XRootDHost");
            if (env)
                rs.hostName = getenv("XRootDHost");
            rs.protocol = HostAccessProtocol::XRootD;
            rs.xrootdTransferProtocol = XRootDTransferProtocol::XRootD;
        }
        if (rs.hostName.empty())
        {
            rs.hostName = "localhost";
        }
    }
    return rs;
}

std::shared_ptr<adios2::Remote> GetRemote(const RemoteSetup &remoteSetup,
                                          const std::string &RemoteFileName,
                                          const adios2::Mode openMode, const bool rowMajorOrdering,
                                          const Params remoteParams)
{
#if defined(ADIOS2_HAVE_CURL) || defined(ADIOS2_HAVE_XROOTD)
    auto lf_getXRootDHostPort = [&](int defaultPort) -> std::tuple<std::string, int> {
        std::string XRootDHost = "localhost";
        int XRootDPort = defaultPort;
        if (remoteSetup.hostConfig)
        {
            XRootDHost = remoteSetup.hostConfig->hostname;
            if (remoteSetup.hostConfig->port > 0)
            {
                XRootDPort = remoteSetup.hostConfig->port;
            }
        }
        else if (remoteSetup.hostName != "localhost")
        {
            auto colon_pos = remoteSetup.hostName.find(':');
            if (colon_pos == std::string::npos)
            {
                XRootDHost = remoteSetup.hostName;
            }
            else
            {
                XRootDHost = remoteSetup.hostName.substr(0, colon_pos);
                try
                {
                    XRootDPort = std::stoi(remoteSetup.hostName.substr(colon_pos + 1));
                }
                catch (...)
                {
                }
            }
        }
        return std::make_tuple(XRootDHost, XRootDPort);
    };
#endif

    std::shared_ptr<adios2::Remote> remote;

    Params params;
    std::string tarinfo = helper::GetParameter("TarInfo", remoteParams, false, "");
    if (!tarinfo.empty())
        params["TarInfo"] = tarinfo;
    std::string selectsteps = helper::GetParameter("SelectSteps", remoteParams, false, "");
    if (!selectsteps.empty())
        params["SelectSteps"] = selectsteps;
    std::string ignoreflattensteps =
        helper::GetParameter("IgnoreFlattenSteps", remoteParams, false, "");
    if (!ignoreflattensteps.empty())
        params["IgnoreFlattenSteps"] = ignoreflattensteps;
    // Send our file id so the server can detect stale cached metadata (0 = none).
    std::string fileuuid = helper::GetParameter("FileUUID", remoteParams, false, "");
    if (!fileuuid.empty())
        params["FileUUID"] = fileuuid;

#if defined(ADIOS2_HAVE_CURL) || defined(ADIOS2_HAVE_XROOTD)
    if (remoteSetup.protocol == HostAccessProtocol::XRootD &&
        (remoteSetup.xrootdTransferProtocol == XRootDTransferProtocol::HTTP ||
         remoteSetup.xrootdTransferProtocol == XRootDTransferProtocol::HTTPS ||
         remoteSetup.xrootdTransferProtocol == XRootDTransferProtocol::XrdCl))
    {
        // XrdCl reaches the origin/federation over HTTPS; the libcurl path
        // additionally supports plain HTTP.
        const bool useXrdCl = (remoteSetup.xrootdTransferProtocol == XRootDTransferProtocol::XrdCl);
        const bool useHttps =
            useXrdCl || (remoteSetup.xrootdTransferProtocol == XRootDTransferProtocol::HTTPS);
        auto tup = lf_getXRootDHostPort(useHttps ? 443 : 80);
        remote = std::make_unique<XrootdHttpRemote>(remoteSetup);
        params["UseHttps"] = useHttps ? "true" : "false";
        if (useXrdCl)
            params["Backend"] = "XrdCl";
        // For testing, disable SSL verification (only relevant for HTTPS)
        if (useHttps) // && getenv("XRootDHttpsNoVerify"))
        {
            params["VerifySSL"] = "false";
        }
        remote->Open(std::get<0>(tup), std::get<1>(tup), RemoteFileName, openMode, rowMajorOrdering,
                     params);
        return remote;
    }
#endif
#ifdef ADIOS2_HAVE_XROOTD
    if (remoteSetup.protocol == HostAccessProtocol::XRootD &&
        remoteSetup.xrootdTransferProtocol == XRootDTransferProtocol::XRootD)
    {
        auto tup = lf_getXRootDHostPort(1094);
        remote = std::make_unique<XrootdRemote>(remoteSetup);
        remote->Open(std::get<0>(tup), std::get<1>(tup), RemoteFileName, openMode, rowMajorOrdering,
                     params);
        return remote;
    }
#endif
#ifdef ADIOS2_HAVE_SST
    if (remoteSetup.protocol == HostAccessProtocol::SSH)
    {
        auto pair = CManagerSingleton::MakeEVPathConnection(remoteSetup);
        remote = pair.first;
        int localPort = pair.second;
        if (remote && localPort > -1)
        {
            remote->Open("localhost", localPort, RemoteFileName, openMode, rowMajorOrdering,
                         params);
        }
        return remote;
    }
#endif
    return remote;
}

} // end namespace adios2
