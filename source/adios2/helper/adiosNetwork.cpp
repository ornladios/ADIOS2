/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosNetwork.cpp implementation of adiosNetwork.h functions
 *
 */

#ifdef _WIN32
#include <winsock2.h> // SOCKET struct
#endif
#include "adios2/helper/adiosComm.h"
#include "adios2/toolkit/transport/file/FileFStream.h"
#include "adiosNetwork.h"

#include <string.h> // memcpy

#ifndef _WIN32

#include <netinet/in.h>
#include <sys/socket.h> //getFQDN

#include <arpa/inet.h>
#include <netdb.h>     //getFQDN
#include <sys/types.h> //getFQDN
#include <unistd.h>    // gethostname
#define SOCKET int

#if defined(ADIOS2_HAVE_DATAMAN) || defined(ADIOS2_HAVE_TABLE)

#include <iostream>
#include <thread>

#include <arpa/inet.h>  //AvailableIpAddresses() inet_ntoa
#include <net/if.h>     //AvailableIpAddresses() struct if_nameindex
#include <netinet/in.h> //AvailableIpAddresses() struct sockaddr_in
#include <nlohmann_json.hpp>
#include <sys/ioctl.h> //AvailableIpAddresses() ioctl

#endif // ADIOS2_HAVE_DATAMAN || ADIOS2_HAVE_TABLE

#else // _WIN32
#ifndef FD_SETSIZE
#define FD_SETSIZE 1024
#endif
#include <process.h>
#include <time.h>
#include <winsock2.h> // SOCKET struct

#include <WS2tcpip.h>
#define getpid() _getpid()
#define read(fd, buf, len) recv(fd, (buf), (len), 0)
#define write(fd, buf, len) send(fd, buf, (len), 0)
#define close(x) closesocket(x)
#define INST_ADDRSTRLEN 50

#include <tchar.h>
#include <windows.h> // GetComputerName
#endif               // _WIN32

namespace adios2
{
namespace helper
{

std::string GetFQDN() noexcept
{
    char hostname[1024];
#ifdef WIN32
    TCHAR infoBuf[1024];
    DWORD bufCharCount = sizeof(hostname);
    memset(hostname, 0, sizeof(hostname));
    if (GetComputerName(infoBuf, &bufCharCount))
    {
        int i;
        for (i = 0; i < sizeof(hostname); i++)
        {
            hostname[i] = infoBuf[i];
        }
    }
    else
    {
        strcpy(hostname, "Unknown_Host_Name");
    }
#else
    struct addrinfo hints, *info, *p;

    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; /*either IPV4 or IPV6*/
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    if (getaddrinfo(hostname, NULL, &hints, &info) == 0)
    {
        for (p = info; p != NULL; p = p->ai_next)
        {
            // printf("hostname: %s\n", p->ai_canonname);
            if (strchr(p->ai_canonname, '.') != NULL)
            {
                strncpy(hostname, p->ai_canonname, sizeof(hostname) - 1);
                break;
            }
        }
    }
    else
    {
        strcpy(hostname, "Unknown_Host_Name");
    }
    freeaddrinfo(info);
#endif
    return std::string(hostname);
}

std::string GetClusterName() noexcept
{
    std::string fqdn = GetFQDN();
    if (fqdn.rfind("login", 0) == 0)
    {
        fqdn.erase(0, fqdn.find('.') + 1);
    }
    if (fqdn.rfind("batch", 0) == 0)
    {
        fqdn.erase(0, fqdn.find('.') + 1);
    }
    return fqdn.substr(0, fqdn.find('.'));
}

#ifndef _WIN32
#if defined(ADIOS2_HAVE_DATAMAN) || defined(ADIOS2_HAVE_TABLE)

#if defined(__clang__)
#if __has_feature(memory_sanitizer)
// Memory Sanitizer fails to recognize that if_nameindex initializes
// the memory in the array behind the pointer it returns.
__attribute__((no_sanitize("memory")))
#endif
#endif
std::vector<std::string>
AvailableIpAddresses() noexcept
{
    std::vector<std::string> ips;
    int socket_handler = -1;
    if ((socket_handler = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return ips;
    }
    struct if_nameindex *head = if_nameindex();
    if (!head)
    {
        close(socket_handler);
        return ips;
    }
    for (struct if_nameindex *p = head; !(p->if_index == 0 && p->if_name == NULL); ++p)
    {
        struct ifreq req;
        strncpy(req.ifr_name, p->if_name, IFNAMSIZ - 1);
        if (ioctl(socket_handler, SIOCGIFADDR, &req) < 0)
        {
            if (errno == EADDRNOTAVAIL)
            {
                continue;
            }
            if_freenameindex(head);
            close(socket_handler);
            return ips;
        }
        const std::string ip = inet_ntoa(((struct sockaddr_in *)&req.ifr_addr)->sin_addr);
        if (ip != "127.0.0.1")
        {
            ips.emplace_back(ip);
        }
    }
    if_freenameindex(head);
    close(socket_handler);
    return ips;
}

void HandshakeWriter(Comm const &comm, size_t &appID, std::vector<std::string> &fullAddresses,
                     const std::string &name, const std::string &engineName, const int basePort,
                     const int channelsPerRank, const int maxRanksPerNode, const int maxAppsPerNode)
{

    int mpiRank = comm.Rank();
    int mpiSize = comm.Size();

    const std::string globalFilename = ".socket";
    const std::string globalLockFilename = ".socket.lock";

    const std::string engineFilename = name + "." + engineName;
    const std::string engineLockFilename = name + "." + engineName + ".lock";

    // Get IP address
    auto ips = helper::AvailableIpAddresses();
    std::string ip = "127.0.0.1";
    if (ips.empty() == false)
    {
        ip = ips[0];
    }

    // Check total number of writer apps
    if (mpiRank == 0)
    {
        transport::FileFStream lockCheck(comm);
        while (true)
        {
            try
            {
                lockCheck.Open(globalLockFilename, Mode::Read);
                lockCheck.Close();
            }
            catch (...)
            {
                break;
            }
        }
        transport::FileFStream lockWrite(comm);
        lockWrite.Open(globalLockFilename, Mode::Write);

        transport::FileFStream numRead(comm);
        try
        {
            numRead.Open(globalFilename, Mode::Read);
            auto size = numRead.GetSize();
            std::vector<char> numAppsChar(size);
            numRead.Read(numAppsChar.data(), numAppsChar.size());
            appID = 1 + stoi(std::string(numAppsChar.begin(), numAppsChar.end()));
            numRead.Close();
        }
        catch (...)
        {
        }
        transport::FileFStream numWrite(comm);
        numWrite.Open(globalFilename, Mode::Write);
        std::string numAppsString = std::to_string(appID);
        numWrite.Write(numAppsString.data(), numAppsString.size());
        numWrite.Close();

        lockWrite.Close();
        remove(globalLockFilename.c_str());
    }

    appID = comm.BroadcastValue(appID);

    // Make full addresses
    for (int i = 0; i < channelsPerRank; ++i)
    {
        std::string addr =
            "tcp://" + ip + ":" +
            std::to_string(basePort + (100 * appID) + (mpiRank % 1000) * channelsPerRank + i) +
            "\0";
        fullAddresses.push_back(addr);
    }
    nlohmann::json localAddressesJson = fullAddresses;
    std::string localAddressesStr = localAddressesJson.dump();
    std::vector<char> localAddressesChar(64 * channelsPerRank, '\0');
    std::memcpy(localAddressesChar.data(), localAddressesStr.c_str(), localAddressesStr.size());
    std::vector<char> globalAddressesChar(64 * channelsPerRank * mpiSize, '\0');
    comm.GatherArrays(localAddressesChar.data(), 64 * channelsPerRank, globalAddressesChar.data());

    // Writing handshake file
    if (mpiRank == 0)
    {
        nlohmann::json globalAddressesJson;
        for (int i = 0; i < mpiSize; ++i)
        {
            auto j = nlohmann::json::parse(&globalAddressesChar[i * 64 * channelsPerRank]);
            for (auto &i : j)
            {
                globalAddressesJson.push_back(i);
            }
        }
        std::string globalAddressesStr = globalAddressesJson.dump();
        transport::FileFStream lockstream(comm);
        lockstream.Open(engineLockFilename, Mode::Write);
        transport::FileFStream ipstream(comm);
        ipstream.Open(engineFilename, Mode::Write);
        ipstream.Write(globalAddressesStr.data(), globalAddressesStr.size());
        ipstream.Close();
        lockstream.Close();
        remove(engineLockFilename.c_str());
    }
}

void HandshakeReader(Comm const &comm, size_t &appID, std::vector<std::string> &fullAddresses,
                     const std::string &name, const std::string &engineName)
{
    const std::string engineLockFilename = name + "." + engineName + ".lock";
    const std::string engineFilename = name + "." + engineName;

    auto ips = helper::AvailableIpAddresses();
    if (ips.empty())
    {
        appID = rand();
    }
    else
    {
        std::hash<std::string> hash_fn;
        appID = hash_fn(ips[0]);
    }
    comm.BroadcastValue(appID);

    transport::FileFStream ipstream(comm);
    while (true)
    {
        try
        {
            ipstream.Open(engineFilename, Mode::Read);
            break;
        }
        catch (...)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    transport::FileFStream lockstream(comm);
    while (true)
    {
        try
        {
            lockstream.Open(engineLockFilename, Mode::Read);
            lockstream.Close();
        }
        catch (...)
        {
            break;
        }
    }

    auto size = ipstream.GetSize();
    std::vector<char> address(size);
    ipstream.Read(address.data(), size);
    ipstream.Close();
    nlohmann::json j = nlohmann::json::parse(address);
    fullAddresses = j.get<std::vector<std::string>>();
}

#endif // ADIOS2_HAVE_DATAMAN || ADIOS2_HAVE_TABLE
#endif // _WIN32

struct NetworkSocketData
{
    sockaddr_in m_Sockaddr;
    SOCKET m_Socket;
};

NetworkSocket::NetworkSocket()
{
    m_Data = new NetworkSocketData();
    m_Data->m_Socket = -1;
};

NetworkSocket::~NetworkSocket() { delete m_Data; };

bool NetworkSocket::valid() const { return (m_Data->m_Socket > 0); }

static sockaddr_in ResolveHostName(std::string m_hostname, uint16_t m_server_port)
{
    sockaddr_in sockaddr;

#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
    struct hostent *hostent = gethostbyname(m_hostname.c_str());
    if (hostent == NULL)
    {
        helper::Throw<std::ios_base::failure>("Helper", "helper:adiosNetwork", "ResolveHostName",
                                              "error: gethostbyname " + m_hostname);
    }

    uint32_t addr_tmp = inet_addr(inet_ntoa(*(struct in_addr *)*(hostent->h_addr_list)));
    if (addr_tmp == INADDR_NONE)
    {
        helper::Throw<std::ios_base::failure>("Helper", "helper:adiosNetwork", "ResolveHostName",
                                              "error: inet_addr " +
                                                  std::string(*(hostent->h_addr_list)));
    }

    sockaddr.sin_addr.s_addr = addr_tmp;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(m_server_port);
    return sockaddr;
}

void NetworkSocket::Connect(std::string hostname, uint16_t port, std::string protocol)
{
    struct protoent *protoent = getprotobyname(protocol.c_str());
    if (protoent == NULL)
    {
        helper::Throw<std::ios_base::failure>("Helper", "helper:adiosNetwork", "ConnectToServer",
                                              "error: Cannot make getprotobyname \"" + protocol +
                                                  "\"");
    }

    m_Data->m_Sockaddr = ResolveHostName(hostname, port);

    m_Data->m_Socket = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
    if (m_Data->m_Socket == -1)
    {
        helper::Throw<std::ios_base::failure>("Helper", "helper:adiosNetwork", "ConnectToServer",
                                              "error: Cannot create socket");
    }

    int result = connect(m_Data->m_Socket, (sockaddr *)&(m_Data->m_Sockaddr), sizeof(sockaddr));

    if (result == -1)
    {
        helper::Throw<std::ios_base::failure>("Helper", "helper:adiosNetwork", "ConnectToServer",
                                              "error: Cannot connect to server at " + hostname +
                                                  ":" + std::to_string(port));
    }
}

void NetworkSocket::RequestResponse(const std::string &request, char *response,
                                    size_t maxResponseSize)
{
#ifdef _WIN32
    int result;
    int len = static_cast<int>(request.length());
    int maxlen = static_cast<int>(maxResponseSize) - 1;
#else
    ssize_t result;
    size_t len = request.length();
    size_t maxlen = maxResponseSize - 1;
#endif
    result = write(m_Data->m_Socket, request.c_str(), len);
    if (result == -1)
    {
        helper::Throw<std::ios_base::failure>("Helper", "helper:adiosNetwork", "RequestResponse",
                                              "error: Cannot send request");
    }

    result = read(m_Data->m_Socket, response, maxlen);
    if (result == -1)
    {
        helper::Throw<std::ios_base::failure>("Helper", "helper:adiosNetwork", "RequestResponse",
                                              "error: Cannot get response");
    }
    // safely null terminate
    response[result] = '\0';
}

void NetworkSocket::Close()
{
    if (m_Data->m_Socket != -1)
    {
        close(m_Data->m_Socket);
        m_Data->m_Socket = -1;
    }
}

} // end namespace helper
} // end namespace adios2
