#include "FileHTTPS.h"
#include <cstring>
#include <fstream>
#include <netdb.h>
#include <unistd.h>

namespace adios2
{
namespace transport
{

void ParseURL(const std::string &url, std::string &hostname, std::string &path)
{
    const std::string httpsPrefix = "https://";
    size_t pos = 0;

    // Skip scheme if present
    if (url.rfind(httpsPrefix, 0) == 0)
        pos = httpsPrefix.size();

    // Find first '/' after hostname
    size_t slashPos = url.find('/', pos);
    if (slashPos == std::string::npos)
    {
        hostname = url.substr(pos);
        path = "";
    }
    else
    {
        hostname = url.substr(pos, slashPos - pos);
        path = url.substr(slashPos);
    }
}

int ConnectTCP(const std::string &hostname, int port)
{
    struct hostent *host = gethostbyname(hostname.c_str());
    if (!host)
    {
        std::cerr << "gethostbyname failed\n";
        exit(1);
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "transport::file::FileHTTPS", "ConnectTCP",
            "cannot create a socket object when connecting to = " + hostname +
                " port = " + std::to_string(port));
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = *(long *)(host->h_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        close(sock);
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileHTTPS", "ConnectTCP",
                                              "cannot make TCP connection to host = " + hostname +
                                                  " port = " + std::to_string(port));
    }
    return sock;
}

std::string ExtractHeaderValue(const std::string &headers, const std::string &key)
{
    size_t pos = headers.find(key);
    if (pos == std::string::npos)
        return "";
    pos += key.size();
    size_t end = headers.find("\r\n", pos);
    return headers.substr(pos, end - pos);
}

FileHTTPS::FileHTTPS(helper::Comm const &comm) : Transport("File", "HTTPS", comm)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
#warning "Using OpenSSL pre-1.1.0 version"
    SSL_library_init();
#else
#warning "Using OpenSSL post-1.1.0 version"
    OPENSSL_init_ssl(0, NULL);
#endif
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    m_sslCtx = SSL_CTX_new(TLS_client_method());
    if (!m_sslCtx)
    {
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileHTTPS", "InitSSL",
                                              "cannot create SSL context");
    }
}

FileHTTPS::~FileHTTPS() { Close(); }

void FileHTTPS::CleanupSSL(SSL *ssl, SOCKET sock)
{
    if (ssl)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    close(sock);
}

void FileHTTPS::WaitForOpen()
{
    if (m_IsOpening)
    {
        m_IsOpening = false;
        CheckFile("couldn't open HTTPS socket " + m_Name);
        m_IsOpen = true;
    }
}

void FileHTTPS::Open(const std::string &name, const Mode openMode, const bool async,
                     const bool directio)
{
    ParseURL(name, m_hostname, m_path);
    std::cout << "FileHTTPS::Open( hostname = " << m_hostname << ", path = " << m_path << ")\n";
    // GetSize();
}

void FileHTTPS::OpenChain(const std::string &name, Mode openMode, const helper::Comm &chainComm,
                          const bool async, const bool directio)
{
    return;
}

void FileHTTPS::Write(const char *buffer, size_t size, size_t start) { return; }

void FileHTTPS::Read(char *buffer, size_t size, size_t start)
{
    const size_t BUF_SIZE = 8192;
    /*        enum CONSTEXPR
        {
            MAX_REQUEST_LEN = 1024
        };
        char request[MAX_REQUEST_LEN] = {'\0'};

        int request_len = snprintf(request, MAX_REQUEST_LEN, request_template.c_str(),
       m_path.c_str(), m_hostname.c_str(), start, start + size - 1);

    */
    std::string request = "GET " + m_path + " HTTP/1.1\r\nHost: " + m_hostname +
                          "\r\nRange: bytes=" + std::to_string(start) + "-" +
                          std::to_string(start + size - 1) + "\r\n\r\n";

    SOCKET sock = ConnectTCP(m_hostname, m_server_port);
    SSL *ssl = SSL_new(m_sslCtx);
    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) <= 0)
    {
        ERR_print_errors_fp(stderr);
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileHTTPS", "Read",
                                              "SSL handshake failed");
    }

    std::cout << "Request: [" << request << "]" << std::endl;
    SSL_write(ssl, request.c_str(), (int)request.size());

    // first we have to read and parse the header to get to the actual data bytes
    bool headerParsed = false;
    std::string response;
    int bytes;
    char buf[BUF_SIZE];
    size_t bytes_recd = 0;
    while (!headerParsed)
    {
        bytes = SSL_read(ssl, buf, sizeof(buf));
        response.append(buf, bytes);
        size_t headerEnd = response.find("\r\n\r\n");
        if (headerEnd != std::string::npos)
        {
            headerParsed = true;
            size_t bodyStart = headerEnd + 4;
            size_t bodyLength = response.size() - bodyStart;
            memcpy(buffer, response.data() + bodyStart, bodyLength);
            bytes_recd = bodyLength;
        }
    }

    while (bytes_recd < size)
    {
        int read_len = (int)((size - bytes_recd) < BUF_SIZE ? (size - bytes_recd) : BUF_SIZE);
        int nbytes = SSL_read(ssl, buffer + bytes_recd, read_len);
        if (nbytes <= 0)
        {
            helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileHTTPS", "Read",
                                                  "SSL_read failed");
        }
        bytes_recd += nbytes;
    }
    std::cout << "Downloaded " << bytes_recd << " bytes into memory.\n";

    std::ofstream out("dump.data", std::ios::binary);
    if (out)
    {
        out.write(buffer, size);
        out.close();
    }
    CleanupSSL(ssl, sock);
}

size_t FileHTTPS::GetSize()
{
    SOCKET sock = ConnectTCP(m_hostname, m_server_port);
    SSL *ssl = SSL_new(m_sslCtx);
    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) <= 0)
    {
        ERR_print_errors_fp(stderr);
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileHTTPS", "GetSize",
                                              "SSL handshake failed");
    }

    std::string request =
        "HEAD " + m_path + " HTTP/1.1\r\nHost: " + m_hostname + "\r\nConnection: close\r\n\r\n";

    // std::cout << "Request: [" << request << "]" << std::endl;
    SSL_write(ssl, request.c_str(), (int)request.size());

    char buffer[4096] = {0};
    int nbytes = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    buffer[nbytes] = '\0';
    if (nbytes <= 0)
    {
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileHTTPS", "GetSize",
                                              "failed to read HEAD response");
    }

    std::string headers(buffer);
    /// std::cout << "Headers: " << headers << "\n";
    std::string cl = ExtractHeaderValue(headers, "Content-Length: ");
    m_fileSize = cl.empty() ? 0 : std::stoul(cl);
    std::cout << "File size: " << m_fileSize << " bytes\n";

    CleanupSSL(ssl, sock);
    return m_fileSize;
}

void FileHTTPS::Flush() {}
void FileHTTPS::Close()
{
    if (m_sslCtx)
    {
        SSL_CTX_free(m_sslCtx);
        m_sslCtx = nullptr;
    }
}

void FileHTTPS::Delete() {}
void FileHTTPS::SeekToEnd() {}
void FileHTTPS::SeekToBegin() {}
void FileHTTPS::Seek(const size_t start) {}
void FileHTTPS::Truncate(const size_t length) {}
void FileHTTPS::MkDir(const std::string &fileName) {}

void FileHTTPS::CheckFile(const std::string hint) const
{
    if (m_fileSize == 0)
    {
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileHTTPS", "CheckFile",
                                              hint + SysErrMsg());
    }
}

std::string FileHTTPS::SysErrMsg() const
{
    return std::string(": errno = " + std::to_string(m_Errno) + ": " + strerror(m_Errno));
}

} // end namespace transport
} // end namespace adios2
