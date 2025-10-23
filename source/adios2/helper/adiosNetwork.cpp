/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosNetwork.cpp implementation of adiosNetwork.h functions
 *
 */

#include "adiosNetwork.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/toolkit/transport/file/FileFStream.h"

#include <string.h> // memcpy

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef FD_SETSIZE
#define FD_SETSIZE 1024
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
// Link with Ws2_32.lib (MSVC)
#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#endif
using socket_t = SOCKET;

inline void close_socket(socket_t s)
{
    if (s != INVALID_SOCKET)
        closesocket(s);
}
inline bool is_valid_socket(socket_t s) { return (s != INVALID_SOCKET); }
// Simple RAII to ensure WSA is initialized once
struct WSAInit
{
    WSAInit()
    {
        WSADATA wsa{};
        int r = WSAStartup(MAKEWORD(2, 2), &wsa);
        if (r != 0)
        {
            throw std::runtime_error("WSAStartup failed: " + std::to_string(r));
        }
    }
    ~WSAInit() { WSACleanup(); }
};

#include <process.h>
#include <time.h>
#define getpid() _getpid()
#define read(fd, buf, len) recv(fd, (buf), (len), 0)
#define write(fd, buf, len) send(fd, buf, (len), 0)
#define close(x) closesocket(x)
#define INST_ADDRSTRLEN 50
#include <tchar.h>
#include <windows.h> // GetComputerName

#else // not _WIN32

#include <arpa/inet.h> //AvailableIpAddresses() inet_ntoa
#include <net/if.h>    //AvailableIpAddresses() struct if_nameindex
#include <netdb.h>
#include <netinet/in.h> //AvailableIpAddresses() struct sockaddr_in
#include <nlohmann_json.hpp>
#include <sys/ioctl.h> //AvailableIpAddresses() ioctl
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
using socket_t = int;
constexpr int INVALID_SOCKET = -1;
inline void close_socket(socket_t s)
{
    if (s >= 0)
        close(s);
}
inline bool is_valid_socket(socket_t s) { return (s >= 0); }

#if defined(ADIOS2_HAVE_DATAMAN) || defined(ADIOS2_HAVE_TABLE)
#include <iostream>
#include <thread>
#endif // ADIOS2_HAVE_DATAMAN || ADIOS2_HAVE_TABLE

#endif // _WIN32

namespace adios2
{
namespace helper
{

std::string GetFQDN() noexcept
{
    char hostname[1024];
#ifdef WIN32
    // Ensure Winsock is initialized (runs only once thanks to static)
    static WSAInit _wsa_guard;
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
            if (p->ai_canonname && (strchr(p->ai_canonname, '.') != NULL))
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
            std::to_string(basePort + (100 * appID) +
                           (mpiRank % 1000) * static_cast<unsigned long>(channelsPerRank) + i) +
            "\0";
        fullAddresses.push_back(addr);
    }
    nlohmann::json localAddressesJson = fullAddresses;
    std::string localAddressesStr = localAddressesJson.dump();
    std::vector<char> localAddressesChar(64 * channelsPerRank, '\0');
    std::memcpy(localAddressesChar.data(), localAddressesStr.c_str(), localAddressesStr.size());
    std::vector<char> globalAddressesChar(static_cast<size_t>(64) * channelsPerRank * mpiSize,
                                          '\0');
    comm.GatherArrays(localAddressesChar.data(), 64 * channelsPerRank, globalAddressesChar.data());

    // Writing handshake file
    if (mpiRank == 0)
    {
        nlohmann::json globalAddressesJson;
        for (int i = 0; i < mpiSize; ++i)
        {
            auto j = nlohmann::json::parse(
                &globalAddressesChar[static_cast<size_t>(i) * static_cast<size_t>(64) *
                                     static_cast<size_t>(channelsPerRank)]);
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
    socket_t m_Socket;
};

NetworkSocket::NetworkSocket()
{
#ifdef _WIN32
    // Ensure Winsock is initialized (runs only once thanks to static)
    static WSAInit _wsa_guard;
#endif

    m_Data = new NetworkSocketData();
    m_Data->m_Socket = INVALID_SOCKET;
};

NetworkSocket::~NetworkSocket() { delete m_Data; };

bool NetworkSocket::valid() const { return is_valid_socket(m_Data->m_Socket); }
int NetworkSocket::GetSocket() { return static_cast<int>(m_Data->m_Socket); }

void NetworkSocket::Connect(const std::string &hostname, uint16_t port, std::string protocol)
{
    struct addrinfo hints
    {
    };
    struct addrinfo *res, *rp;
    m_Data->m_Socket = INVALID_SOCKET;
    int ret;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_ADDRCONFIG; // Prefer addresses that are configured

    std::string portStr = std::to_string(port);
    ret = getaddrinfo(hostname.c_str(), portStr.c_str(), &hints, &res);
    if (ret != 0)
    {
        helper::Throw<std::ios_base::failure>("Helper", "helper::adiosNetwork", "Connect",
                                              "getaddrinfo failed :" +
                                                  std::string(gai_strerror(ret)));
    }

    // Try each address until we successfully connect
    for (rp = res; rp != nullptr; rp = rp->ai_next)
    {
        socket_t s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (s == INVALID_SOCKET)
            continue; // Try next address

        if (connect(s, rp->ai_addr, static_cast<socklen_t>(rp->ai_addrlen)) == 0)
        {
            m_Data->m_Socket = s; // Success
            break;
        }
        close_socket(s);
    }

    freeaddrinfo(res);

    if (m_Data->m_Socket == INVALID_SOCKET)
    {
        helper::Throw<std::ios_base::failure>("Helper", "helper::adiosNetwork", "Connect",
                                              "cannot make TCP connection to host = " + hostname +
                                                  " port = " + std::to_string(port));
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

#ifdef ADIOS2_HAVE_OPENSSL

#include <openssl/err.h>
#include <openssl/ssl.h>

struct SSLData
{
    NetworkSocket m_Socket;
    SSL *m_SSL;
    SSL_CTX *m_sslCtx = nullptr;
};

SSLSocket::SSLSocket()
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    SSL_library_init();
#else
    OPENSSL_init_ssl(0, NULL);
#endif
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    m_Data = new SSLData();
    m_Data->m_sslCtx = SSL_CTX_new(TLS_client_method());
    if (!m_Data->m_sslCtx)
    {
        helper::Throw<std::ios_base::failure>("Helper", "helper::adiosNetwork::SSLSocket",
                                              "SSLSocket", "cannot create SSL context");
    }
};

SSLSocket::~SSLSocket()
{
    if (m_Data->m_sslCtx)
    {
        SSL_CTX_free(m_Data->m_sslCtx);
        m_Data->m_sslCtx = nullptr;
    };
    delete m_Data;
};

bool SSLSocket::valid() const
{
    return (m_Data->m_Socket.valid() && m_Data->m_sslCtx != nullptr && m_Data->m_SSL != nullptr);
};

void SSLSocket::Connect(const std::string &hostname, uint16_t port, std::string protocol)
{
    m_Data->m_Socket.Connect(hostname, port);
    m_Data->m_SSL = SSL_new(m_Data->m_sslCtx);
    SSL_set_fd(m_Data->m_SSL, m_Data->m_Socket.GetSocket());

    if (SSL_connect(m_Data->m_SSL) <= 0)
    {
        ERR_print_errors_fp(stderr);
        helper::Throw<std::ios_base::failure>("Helper", "helper::adiosNetwork::SSLSocket",
                                              "Connect", "SSL handshake failed");
    }
}

int SSLSocket::Write(const char *buffer, int size)
{
    return SSL_write(m_Data->m_SSL, buffer, size);
}
int SSLSocket::Read(char *buffer, int size) { return SSL_read(m_Data->m_SSL, buffer, size); }

void SSLSocket::Close()
{
    if (m_Data->m_SSL)
    {
        SSL_shutdown(m_Data->m_SSL);
        SSL_free(m_Data->m_SSL);
        m_Data->m_SSL = nullptr;
    }
    m_Data->m_Socket.Close();
};

#endif // ADIOS2_HAVE_OPENSSL

} // end namespace helper
} // end namespace adios2
