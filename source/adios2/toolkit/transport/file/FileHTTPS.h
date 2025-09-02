#ifndef ADIOS2_FILEHTTPS_H
#define ADIOS2_FILEHTTPS_H

#ifdef _WIN32
#ifndef FD_SET_SIZE
#define FD_SETSIZE 1024
#endif
#include <winsock2.h>
#endif
#include "../Transport.h"
#include "adios2/common/ADIOSConfig.h"
#ifdef _MSC_VER
#include <process.h>
#include <time.h>
#include <windows.h>
#define getpid() _getpid()
#else
#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#define SOCKET int
#endif

#include <openssl/err.h>
#include <openssl/ssl.h>

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

    void CleanupSSL(SSL *ssl, SOCKET sock);
    void CheckFile(const std::string hint) const;
    void WaitForOpen();
    std::string SysErrMsg() const;
};

} // end namespace transport
} // end namespace adios2

#endif // ADIOS2_FILEHTTPS_H
