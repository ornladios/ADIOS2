#ifndef ADIOS2_FILEHTTPS_H
#define ADIOS2_FILEHTTPS_H

#include "../Transport.h"
#include "./FileFStream.h"
#include "adios2/common/ADIOSConfig.h"

#include <openssl/err.h>
#include <openssl/ssl.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
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
#else // not _WIN32
#include <arpa/inet.h>
#include <netdb.h>
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
#endif // _WIN32

namespace adios2
{
namespace helper
{
class Comm;
}
namespace transport
{

/** File descriptor transport using the POSIX IO library over HTTPS */
class FileHTTPS : public Transport
{

public:
    FileHTTPS(helper::Comm const &comm);

    ~FileHTTPS();

    void SetParameters(const Params &parameters);

    void Open(const std::string &name, const Mode openMode, const bool async = false,
              const bool directio = false) final;

    void OpenChain(const std::string &name, Mode openMode, const helper::Comm &chainComm,
                   const bool async = false, const bool directio = false) final;

    void Write(const char *buffer, size_t size, size_t start = MaxSizeT) final;

    void Read(char *buffer, size_t size, size_t start = MaxSizeT) final;

    size_t GetSize() final;

    void Flush() final;

    void Close() final;

    void Delete() final;

    void SeekToEnd() final;

    void SeekToBegin() final;

    void Seek(const size_t start = MaxSizeT) final;

    size_t CurrentPos() final { return 0; };

    void Truncate(const size_t length) final;

    void MkDir(const std::string &fileName) final;

private:
    std::string m_hostname, m_path;
    int m_server_port = 443; // HTTPS default

    SSL_CTX *m_sslCtx = nullptr;

    int m_Errno = 0;
    bool m_IsOpening = false;

    std::string request_template = "GET %s HTTP/1.1\r\nHost: %s\r\nRange: bytes=%d-%d\r\n\r\n";

    size_t m_fileSize = 0;

    void CleanupSSL(SSL *ssl, socket_t sock);
    void CheckFile(const std::string hint) const;
    void WaitForOpen();
    std::string SysErrMsg() const;

    size_t m_SeekPos = 0;
    size_t m_Size = 0;
    int m_Verbose = 0;
    bool m_RecheckMetadata = true; // always check if cached metadata is complete

    void SetUpCache();
    void CheckCache(const size_t fileSize);
    std::string m_CachePath;        // local cache directory
    bool m_CachingThisFile = false; // save content to local cache
    FileFStream *m_CacheFileWrite;
    bool m_IsCached = false; // true if file is already in cache
    FileFStream *m_CacheFileRead;
    std::string m_CacheFilePath; // full path to file in cache
};

} // end namespace transport
} // end namespace adios2

#endif // ADIOS2_FILEHTTPS_H
