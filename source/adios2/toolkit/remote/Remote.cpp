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

void Remote::OpenReadSimpleFile(const std::string hostname, const int32_t port,
                                const std::string filename, std::vector<char> &contents)
{
    ThrowUp("RemoteSimpleOpenRead");
};

Remote::GetHandle Remote::Get(const char *VarName, size_t Step, size_t StepCount, size_t BlockID,
                              Dims &Count, Dims &Start, Accuracy &accuracy, void *dest)
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

} // end namespace adios2
