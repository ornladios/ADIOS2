/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 */
#include "Remote.h"
#include "EVPathRemote.h"
#include "adios2/core/ADIOS.h"
#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosNetwork.h"
#include "adios2/helper/adiosString.h"
#include "adios2/helper/adiosSystem.h"
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
                  const Mode mode, bool RowMajorOrdering)
{
    ThrowUp(("RemoteOpen"));
};

void Remote::OpenSimpleFile(const std::string hostname, const int32_t port,
                            const std::string filename)
{
    ThrowUp("RemoteSimpleOpen");
};

Remote::GetHandle Remote::Get(char *VarName, size_t Step, size_t BlockID, Dims &Count, Dims &Start,
                              void *dest)
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
Remote::~Remote() {}
Remote::Remote(const adios2::HostOptions &hostOptions)
: m_HostOptions(std::make_shared<adios2::HostOptions>(hostOptions))
{
}

int Remote::LaunchRemoteServerViaConnectionManager(const std::string remoteHost)
{
    if (remoteHost.empty() || remoteHost == "localhost")
    {
        // std::cout << "Remote::LaunchRemoteServerViaConnectionManager: Assume server is already "
        //              "running at on localhost at port = "
        //           << 26200 << std::endl;
        return 26200;
    }

    helper::NetworkSocket socket;
    socket.Connect("localhost", 30000);

    struct adios2::HostConfig *hostconf = nullptr;

    auto it = m_HostOptions->find(remoteHost);
    if (it != m_HostOptions->end())
    {
        for (auto &ho : it->second)
        {
            if (ho.protocol == HostAccessProtocol::SSH)
            {
                hostconf = &ho;
            }
        }
    }
    if (!hostconf)
    {
        helper::Throw<std::invalid_argument>("Toolkit", "Remote", "EstablishConnection",
                                             "No ssh configuration found for host " + remoteHost +
                                                 ". Add config in ~/.config/adios2/hosts.yaml");
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

} // end namespace adios2
